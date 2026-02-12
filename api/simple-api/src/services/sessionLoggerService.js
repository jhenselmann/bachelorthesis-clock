const pool = require('../db/pool');

class SessionLoggerService {
  async createSession({ sessionId, language = 'de', sampleRate = 16000, senderId, remoteIp, userAgent }) {
    try {
      const result = await pool.query(
        `INSERT INTO sessions (session_id, language, sample_rate, sender_id, remote_ip, user_agent, status)
         VALUES ($1, $2, $3, $4, $5, $6, 'active') RETURNING *`,
        [sessionId, language, sampleRate, senderId || sessionId, remoteIp, userAgent]
      );
      return result.rows[0];
    } catch (err) {
      console.error('[SessionLogger] createSession failed:', err.message);
      return null;
    }
  }

  async endSession(sessionId, status = 'completed', errorMessage = null) {
    try {
      const result = await pool.query(
        `UPDATE sessions SET ended_at = CURRENT_TIMESTAMP, status = $1, error_message = $2
         WHERE session_id = $3 RETURNING *`,
        [status, errorMessage, sessionId]
      );
      return result.rows[0];
    } catch (err) {
      console.error('[SessionLogger] endSession failed:', err.message);
      return null;
    }
  }

  async logInteraction(data) {
    try {
      await pool.query(
        `INSERT INTO interactions (
          session_id, audio_input_size, transcribed_text,
          rasa_response, rasa_intent, rasa_confidence,
          tts_audio_size, total_processing_time_ms, error_occurred, error_message
        ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)`,
        [
          data.sessionId, data.audioInputSize, data.transcribedText,
          data.rasaResponse, data.rasaIntent, data.rasaConfidence,
          data.ttsAudioSize, data.totalProcessingTimeMs,
          data.errorOccurred || false, data.errorMessage
        ]
      );
    } catch (err) {
      console.error('[SessionLogger] logInteraction failed:', err.message);
    }
  }

  async getSession(sessionId) {
    try {
      const result = await pool.query('SELECT * FROM sessions WHERE session_id = $1', [sessionId]);
      return result.rows[0] || null;
    } catch {
      return null;
    }
  }

  async getSessions({ limit = 50, offset = 0, language, status, startDate, endDate } = {}) {
    try {
      const conditions = ['1=1'];
      const values = [];
      let i = 1;

      if (language) { conditions.push(`language = $${i++}`); values.push(language); }
      if (status) { conditions.push(`status = $${i++}`); values.push(status); }
      if (startDate) { conditions.push(`started_at >= $${i++}`); values.push(startDate); }
      if (endDate) { conditions.push(`started_at <= $${i++}`); values.push(endDate); }

      const where = conditions.join(' AND ');

      const [sessions, count] = await Promise.all([
        pool.query(
          `SELECT * FROM sessions WHERE ${where} ORDER BY started_at DESC LIMIT $${i++} OFFSET $${i}`,
          [...values, limit, offset]
        ),
        pool.query(`SELECT COUNT(*) FROM sessions WHERE ${where}`, values)
      ]);

      return {
        sessions: sessions.rows,
        total: parseInt(count.rows[0].count),
        limit,
        offset
      };
    } catch (err) {
      console.error('[SessionLogger] getSessions failed:', err.message);
      throw err;
    }
  }

  async getInteractions(sessionId) {
    try {
      const result = await pool.query(
        'SELECT * FROM interactions WHERE session_id = $1 ORDER BY interaction_timestamp ASC',
        [sessionId]
      );
      return result.rows;
    } catch {
      return [];
    }
  }

  async getStatistics(days = 30) {
    try {
      const [sessions, interactions] = await Promise.all([
        pool.query('SELECT * FROM session_statistics WHERE date >= CURRENT_DATE - $1::INTEGER ORDER BY date DESC', [days]),
        pool.query('SELECT * FROM interaction_statistics WHERE date >= CURRENT_DATE - $1::INTEGER ORDER BY date DESC', [days])
      ]);
      return { sessions: sessions.rows, interactions: interactions.rows };
    } catch (err) {
      console.error('[SessionLogger] getStatistics failed:', err.message);
      throw err;
    }
  }

  async getActiveSessionsCount() {
    try {
      const result = await pool.query("SELECT COUNT(*) FROM sessions WHERE status = 'active'");
      return parseInt(result.rows[0].count);
    } catch {
      return 0;
    }
  }
}

module.exports = new SessionLoggerService();
