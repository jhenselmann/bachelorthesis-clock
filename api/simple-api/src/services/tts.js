const axios = require('axios');

class TtsService {
  constructor(apiKey, voice = 'alloy') {
    this.apiKey = apiKey;
    this.voice = voice;
    if (!this.apiKey) throw new Error('OpenAI API Key fehlt');
  }

  async synthesize(text) {
    const response = await axios.post(
      'https://api.openai.com/v1/audio/speech',
      {
        model: 'gpt-4o-mini-tts',
        input: text,
        voice: this.voice,
        response_format: 'mp3',
        speed: 1.0
      },
      {
        headers: { Authorization: `Bearer ${this.apiKey}`, 'Content-Type': 'application/json' },
        responseType: 'arraybuffer',
        timeout: 30000
      }
    );

    return Buffer.from(response.data);
  }
}

module.exports = TtsService;
