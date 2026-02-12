const MESSAGE_TYPES = {
  START_RECORDING: 'start_recording',
  STOP_RECORDING: 'stop_recording',
  HEARTBEAT: 'heartbeat',
  TRANSCRIPT: 'transcript',
  RESPONSE_START: 'response_start',
  RESPONSE_END: 'response_end',
  CLIENT_COMPLETE: 'client_complete',
  ERROR: 'error',
  SESSION_INFO: 'session_info'
};

function parseMessage(data) {
  try {
    return { ok: true, data: JSON.parse(data) };
  } catch {
    return { ok: false, error: 'Ungültiges JSON' };
  }
}

function createMessage(type, payload = {}) {
  return JSON.stringify({ type, timestamp: new Date().toISOString(), ...payload });
}

function sessionInfo(sessionId) {
  return createMessage(MESSAGE_TYPES.SESSION_INFO, { sessionId });
}

function transcript(text) {
  return createMessage(MESSAGE_TYPES.TRANSCRIPT, { text });
}

function responseStart(intent, entities, confidence) {
  const payload = { message: 'Audio wird gesendet' };
  if (intent) payload.action = intent;
  if (confidence) payload.confidence = confidence;
  if (entities && Object.keys(entities).length > 0) payload.entities = entities;
  return createMessage(MESSAGE_TYPES.RESPONSE_START, payload);
}

function responseEnd() {
  return createMessage(MESSAGE_TYPES.RESPONSE_END, { message: 'Audio komplett' });
}

function error(errorMessage) {
  return createMessage(MESSAGE_TYPES.ERROR, { error: errorMessage });
}

function heartbeat() {
  return createMessage(MESSAGE_TYPES.HEARTBEAT, { message: 'pong' });
}

function isBinary(data) {
  return Buffer.isBuffer(data) || data instanceof ArrayBuffer;
}

module.exports = {
  MESSAGE_TYPES,
  parseMessage,
  createMessage,
  sessionInfo,
  transcript,
  responseStart,
  responseEnd,
  error,
  heartbeat,
  isBinary
};
