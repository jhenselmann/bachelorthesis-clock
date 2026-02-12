const fs = require('fs');
const path = require('path');
const sessions = require('../services/sessions');
const msg = require('../services/messages');
const { pcmToWav, detectAudioFormat } = require('../utils/audio-utils');
const { normalizeEntities } = require('../utils/time-parser');
const SessionLoggerService = require('../services/sessionLoggerService');

const AUDIO_DIR = path.join(__dirname, '../../audio/incoming');

class VoiceHandler {
  constructor(voicePipeline) {
    this.pipeline = voicePipeline;
  }

  handleConnection(ws, req) {
    const session = sessions.createSession(ws);
    const ip = req.socket.remoteAddress;

    console.log(`[WS] Verbunden: ${session.sessionId} (${ip})`);

    SessionLoggerService.createSession({
      sessionId: session.sessionId,
      language: 'de',
      sampleRate: 16000,
      senderId: session.sessionId,
      remoteIp: ip,
      userAgent: req.headers['user-agent'] || 'unknown'
    }).catch(() => {});

    this.send(ws, msg.sessionInfo(session.sessionId));

    ws.on('message', data => this.handleMessage(ws, session, data));
    ws.on('close', (code, reason) => this.handleClose(session, code, reason));
    ws.on('error', err => this.handleError(session, err));
    ws.on('pong', () => { session.lastHeartbeat = Date.now(); });

    session.pingInterval = setInterval(() => {
      if (ws.readyState === 1) ws.ping();
    }, 30000);
  }

  async handleMessage(ws, session, data) {
    try {
      if (msg.isBinary(data)) {
        if (data.length < 200) {
          const text = data.toString('utf8');
          if (text.startsWith('{') && text.includes('"type"')) {
            await this.handleControlMessage(ws, session, text);
            return;
          }
        }
        this.handleAudio(session, data);
        return;
      }

      await this.handleControlMessage(ws, session, data);
    } catch (err) {
      console.error(`[WS] Fehler: ${session.sessionId}`, err.message);
      this.send(ws, msg.error('Verarbeitung fehlgeschlagen'));
    }
  }

  async handleControlMessage(ws, session, data) {
    const parsed = msg.parseMessage(data);
    if (!parsed.ok) {
      this.send(ws, msg.error(parsed.error));
      return;
    }

    const message = parsed.data;

    switch (message.type) {
      case msg.MESSAGE_TYPES.START_RECORDING:
        session.isRecording = true;
        session.audioBuffer = Buffer.alloc(0);
        if (message.alarm_count !== undefined) session.alarmCount = message.alarm_count;
        if (message.alarms) session.alarms = message.alarms;
        console.log(`[WS] Aufnahme gestartet: ${session.sessionId}`);
        break;

      case msg.MESSAGE_TYPES.STOP_RECORDING:
        session.isRecording = false;
        console.log(`[WS] Aufnahme gestoppt: ${session.sessionId} (${session.audioBuffer.length} bytes)`);
        if (session.audioBuffer.length > 0) {
          await this.processAudio(ws, session);
        }
        break;

      case msg.MESSAGE_TYPES.CLIENT_COMPLETE:
        console.log(`[WS] Client fertig: ${session.sessionId}`);
        break;

      case msg.MESSAGE_TYPES.HEARTBEAT:
        session.lastHeartbeat = Date.now();
        this.send(ws, msg.heartbeat());
        break;
    }
  }

  handleAudio(session, data) {
    if (!session.isRecording) return;
    session.audioBuffer = Buffer.concat([session.audioBuffer, Buffer.from(data)]);
  }

  async processAudio(ws, session) {
    const startTime = Date.now();

    try {
      const format = detectAudioFormat(session.audioBuffer);
      let audioBuffer = session.audioBuffer;
      if (format === 'pcm') {
        audioBuffer = pcmToWav(session.audioBuffer, 16000, 1, 16);
      }

      this.saveAudio(session.sessionId, audioBuffer);

      const metadata = {
        locale: 'de_DE',
        timezone: 'Europe/Berlin',
        alarm_count: session.alarmCount,
        alarms: session.alarms
      };

      const result = await this.pipeline.process(
        audioBuffer,
        session.sessionId,
        metadata,
        text => {
          console.log(`[WS] Transkript: "${text}"`);
          this.send(ws, msg.transcript(text));
        }
      );

      const entities = result.entities ? normalizeEntities(result.entities) : null;

      SessionLoggerService.logInteraction({
        sessionId: session.sessionId,
        audioInputSize: session.audioBuffer.length,
        transcribedText: result.text,
        rasaResponse: result.botResponse,
        rasaIntent: result.intent,
        rasaConfidence: result.confidence,
        rasaEntities: result.entities,
        ttsAudioSize: result.audio?.length || 0,
        totalProcessingTimeMs: Date.now() - startTime,
        errorOccurred: false
      }).catch(() => {});

      await this.sendAudioResponse(ws, session, result.audio, result.intent, entities, result.confidence);
      session.audioBuffer = Buffer.alloc(0);

    } catch (err) {
      console.error(`[WS] Verarbeitung fehlgeschlagen: ${session.sessionId}`, err.message);

      SessionLoggerService.logInteraction({
        sessionId: session.sessionId,
        audioInputSize: session.audioBuffer.length,
        totalProcessingTimeMs: Date.now() - startTime,
        errorOccurred: true,
        errorMessage: err.message
      }).catch(() => {});

      this.send(ws, msg.error('Verarbeitung fehlgeschlagen'));

      setTimeout(() => {
        if (ws.readyState === 1) ws.close(1011, 'Fehler');
      }, 100);
    }
  }

  async sendAudioResponse(ws, session, audio, intent, entities, confidence) {
    if (ws.readyState !== 1) {
      console.warn(`[WS] Verbindung geschlossen: ${session.sessionId}`);
      return;
    }

    console.log(`[WS] Sende Audio: ${session.sessionId} (${audio.length} bytes, intent: ${intent || 'none'})`);

    this.send(ws, msg.responseStart(intent, entities, confidence));
    ws.send(audio);
    this.send(ws, msg.responseEnd());
  }

  handleClose(session, code, reason) {
    console.log(`[WS] Getrennt: ${session.sessionId} (code: ${code})`);

    const status = code === 1000 ? 'completed' : (code === 1006 ? 'timeout' : 'error');
    SessionLoggerService.endSession(session.sessionId, status, reason).catch(() => {});

    if (session.pingInterval) clearInterval(session.pingInterval);
    sessions.deleteSession(session.sessionId);
  }

  handleError(session, err) {
    console.error(`[WS] Fehler: ${session.sessionId}`, err.message);
    SessionLoggerService.endSession(session.sessionId, 'error', err.message).catch(() => {});
  }

  saveAudio(sessionId, buffer) {
    try {
      if (!fs.existsSync(AUDIO_DIR)) fs.mkdirSync(AUDIO_DIR, { recursive: true });
      fs.writeFileSync(path.join(AUDIO_DIR, `${sessionId}.wav`), buffer);
    } catch {
    }
  }

  send(ws, message) {
    if (ws.readyState === 1) ws.send(message);
  }
}

module.exports = VoiceHandler;
