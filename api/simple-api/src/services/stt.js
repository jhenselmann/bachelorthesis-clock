const axios = require('axios');
const FormData = require('form-data');

class SttService {
  constructor(apiKey) {
    this.apiKey = apiKey;
    if (!this.apiKey) throw new Error('OpenAI API Key fehlt');
  }

  async transcribe(audioBuffer) {
    const formData = new FormData();
    formData.append('file', audioBuffer, { filename: 'audio.wav', contentType: 'audio/wav' });
    formData.append('model', 'whisper-1');
    formData.append('language', 'de');

    const response = await axios.post(
      'https://api.openai.com/v1/audio/transcriptions',
      formData,
      {
        headers: { ...formData.getHeaders(), Authorization: `Bearer ${this.apiKey}` },
        timeout: 30000
      }
    );

    return response.data.text || '';
  }
}

module.exports = SttService;
