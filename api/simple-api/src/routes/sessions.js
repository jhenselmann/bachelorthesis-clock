const express = require('express');
const router = express.Router();
const fs = require('fs');
const path = require('path');
const SessionLoggerService = require('../services/sessionLoggerService');

const AUDIO_DIR = path.join(__dirname, '../../audio/incoming');

router.get('/', async (req, res) => {
  try {
    const { limit = 50, offset = 0, language, status, startDate, endDate } = req.query;

    const result = await SessionLoggerService.getSessions({
      limit: parseInt(limit),
      offset: parseInt(offset),
      language,
      status,
      startDate,
      endDate
    });

    res.json({
      success: true,
      data: result.sessions,
      pagination: {
        total: result.total,
        limit: result.limit,
        offset: result.offset,
        hasMore: (result.offset + result.limit) < result.total
      }
    });
  } catch (error) {
    console.error('Sessions API error:', error.message);
    res.status(500).json({ success: false, error: 'Failed to retrieve sessions' });
  }
});

router.get('/active', async (req, res) => {
  try {
    const count = await SessionLoggerService.getActiveSessionsCount();
    res.json({ success: true, data: { count } });
  } catch (error) {
    res.status(500).json({ success: false, error: 'Failed to get active count' });
  }
});

router.get('/statistics/overview', async (req, res) => {
  try {
    const { days = 30 } = req.query;
    const stats = await SessionLoggerService.getStatistics(parseInt(days));
    res.json({ success: true, data: stats });
  } catch (error) {
    res.status(500).json({ success: false, error: 'Failed to get statistics' });
  }
});

router.get('/:sessionId', async (req, res) => {
  try {
    const session = await SessionLoggerService.getSession(req.params.sessionId);
    if (!session) {
      return res.status(404).json({ success: false, error: 'Session not found' });
    }
    res.json({ success: true, data: session });
  } catch (error) {
    res.status(500).json({ success: false, error: 'Failed to retrieve session' });
  }
});

router.get('/:sessionId/interactions', async (req, res) => {
  try {
    const interactions = await SessionLoggerService.getInteractions(req.params.sessionId);
    res.json({ success: true, data: interactions });
  } catch (error) {
    res.status(500).json({ success: false, error: 'Failed to retrieve interactions' });
  }
});

router.get('/:sessionId/audio', async (req, res) => {
  try {
    const audioPath = path.join(AUDIO_DIR, `${req.params.sessionId}.wav`);
    if (!fs.existsSync(audioPath)) {
      return res.status(404).json({ success: false, error: 'Audio not found' });
    }
    res.setHeader('Content-Type', 'audio/wav');
    fs.createReadStream(audioPath).pipe(res);
  } catch (error) {
    res.status(500).json({ success: false, error: 'Failed to retrieve audio' });
  }
});

module.exports = router;
