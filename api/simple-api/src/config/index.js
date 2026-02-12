require('dotenv').config();

const config = {
  port: process.env.PORT || 8000,
  apiToken: process.env.API_TOKEN,
  openaiApiKey: process.env.OPENAI_API_KEY,
  ttsVoice: process.env.OPENAI_TTS_VOICE,
  rasaUrl: process.env.RASA_URL,
  database: {
    host: process.env.POSTGRES_SERVER,
    port: process.env.POSTGRES_PORT,
    name: process.env.POSTGRES_DB,
    user: process.env.POSTGRES_USER,
    password: process.env.POSTGRES_PASSWORD
  }
};

module.exports = config;
