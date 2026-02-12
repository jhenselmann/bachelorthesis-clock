const sessions = new Map();

function createSession(ws) {
  const sessionId = `session_${Date.now()}_${Math.random().toString(36).substring(2, 9)}`;

  const session = {
    ws,
    sessionId,
    audioBuffer: Buffer.alloc(0),
    isRecording: false,
    lastHeartbeat: Date.now(),
    alarms: [],
    alarmCount: 0
  };

  sessions.set(sessionId, session);
  return session;
}

function getSession(sessionId) {
  return sessions.get(sessionId);
}

function deleteSession(sessionId) {
  sessions.delete(sessionId);
}

function getSessionCount() {
  return sessions.size;
}

function startHeartbeatMonitor() {
  setInterval(() => {
    const now = Date.now();
    const timeout = 60000;

    for (const [sessionId, session] of sessions.entries()) {
      if (now - session.lastHeartbeat > timeout) {
        console.log(`[Sessions] Timeout: ${sessionId}`);
        if (session.ws.readyState === 1) session.ws.close(1000, 'Timeout');
        sessions.delete(sessionId);
      }
    }
  }, 30000);
}

module.exports = {
  createSession,
  getSession,
  deleteSession,
  getSessionCount,
  startHeartbeatMonitor
};
