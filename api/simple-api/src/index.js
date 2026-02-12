const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const cors = require('cors');

const config = require('./config');
const { SttService, TtsService, RasaClient, VoicePipeline } = require('./services');
const sessions = require('./services/sessions');
const VoiceHandler = require('./controllers/voiceHandler');
const { validateWebSocketAuth, validateApiAuth } = require('./middleware');

const app = express();
app.use(cors({ origin: '*', credentials: true }));
app.use(express.json());

// Services
const sttService = new SttService(config.openaiApiKey);
const ttsService = new TtsService(config.openaiApiKey, config.ttsVoice);
const rasaClient = new RasaClient(config.rasaUrl);
const voicePipeline = new VoicePipeline(sttService, rasaClient, ttsService);

// Routes
const healthRoutes = require('./routes/health');
const sessionsRoutes = require('./routes/sessions');

app.use('/api/health', healthRoutes);
app.use('/api/sessions', validateApiAuth, sessionsRoutes);

// Server
const server = http.createServer(app);
const wss = new WebSocket.Server({ noServer: true });
const voiceHandler = new VoiceHandler(voicePipeline);

sessions.startHeartbeatMonitor();

// WebSocket Upgrade
server.on('upgrade', (request, socket, head) => {
  if (request.url === '/voice-ws') {
    const auth = validateWebSocketAuth(request);
    if (!auth.valid) {
      socket.write(`HTTP/1.1 ${auth.code} ${auth.error}\r\n\r\n`);
      socket.destroy();
      return;
    }

    wss.handleUpgrade(request, socket, head, ws => {
      voiceHandler.handleConnection(ws, request);
    });
  } else {
    socket.destroy();
  }
});

server.listen(config.port, () => {
  console.log(`\nSmUhr Voice API auf Port ${config.port}`);
  console.log(`  WS  /voice-ws     - Sprach-WebSocket`);
  console.log(`  GET /api/health   - Health Check`);
  console.log(`  GET /api/sessions - Session-Logs\n`);
});
