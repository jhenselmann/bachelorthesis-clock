const ffmpeg = require('fluent-ffmpeg');
const { Readable, PassThrough } = require('stream');

class VoicePipeline {
  constructor(sttService, rasaClient, ttsService) {
    this.stt = sttService;
    this.rasa = rasaClient;
    this.tts = ttsService;
  }

  async process(audioBuffer, sessionId, metadata, onTranscript) {
    // STT
    const text = await this.stt.transcribe(audioBuffer);
    if (onTranscript) onTranscript(text);

    // Rasa
    const rasaResult = await this.rasa.processMessage(text, sessionId, metadata);

    // TTS
    const responseText = rasaResult.botResponse || 'Das habe ich nicht verstanden.';
    let audio = await this.tts.synthesize(responseText);

    // Audio für ESP32 konvertieren (44.1kHz Stereo)
    audio = await this.convertAudio(audio);

    return {
      audio,
      text,
      botResponse: rasaResult.botResponse,
      intent: rasaResult.intent,
      confidence: rasaResult.confidence,
      entities: rasaResult.entities
    };
  }

  convertAudio(audioBuffer) {
    return new Promise((resolve, reject) => {
      const input = new Readable();
      input.push(audioBuffer);
      input.push(null);

      const output = new PassThrough();
      const chunks = [];

      output.on('data', chunk => chunks.push(chunk));
      output.on('end', () => resolve(Buffer.concat(chunks)));

      ffmpeg(input)
        .audioFrequency(44100)
        .audioChannels(2)
        .audioBitrate('128k')
        .format('mp3')
        .on('error', reject)
        .pipe(output, { end: true });
    });
  }
}

module.exports = VoicePipeline;
