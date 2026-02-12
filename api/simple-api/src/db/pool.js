/**
 * Database Connection Pool
 * Manages PostgreSQL connections for session logging
 */

const { Pool } = require('pg');
const config = require('../config');
const Logger = require('../utils/logger');

const pool = new Pool({
  host: config.database.host,
  port: parseInt(config.database.port) || 5432,
  database: config.database.name,
  user: config.database.user,
  password: config.database.password,
  max: 20,
  idleTimeoutMillis: 30000,
  connectionTimeoutMillis: 2000,
  options: '-c search_path=smuhr_dashboard,public'
});

// Test connection on startup
pool.connect((err, client, release) => {
  if (err) {
    Logger.error('Database', 'Failed to connect to PostgreSQL', err);
  } else {
    Logger.info('Database', 'PostgreSQL connection pool established');
    release();
  }
});

// Handle pool errors
pool.on('error', (err) => {
  Logger.error('Database', 'Unexpected error on idle client', err);
});

module.exports = pool;
