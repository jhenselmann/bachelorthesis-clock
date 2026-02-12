const axios = require('axios');

class RasaClient {
  constructor(rasaUrl) {
    this.url = rasaUrl;
    if (!this.url) throw new Error('Rasa URL nicht konfiguriert');
  }

  async processMessage(text, sessionId, metadata = {}) {
    const response = await axios.post(
      `${this.url}/webhooks/rest/webhook`,
      { sender: sessionId, message: text, metadata },
      { timeout: 30000 }
    );

    let botResponse = '';
    if (response.data?.length > 0) {
      botResponse = response.data[0].text || '';
    }

    let intent = null;
    let confidence = null;
    let entities = {};

    try {
      const parseResponse = await axios.post(
        `${this.url}/model/parse`,
        { text },
        { timeout: 30000 }
      );

      if (parseResponse.data?.intent) {
        intent = parseResponse.data.intent.name;
        confidence = parseResponse.data.intent.confidence;
      }

      if (parseResponse.data?.entities) {
        for (const e of parseResponse.data.entities) {
          entities[e.entity] = e.value;
        }
      }
    } catch {
    }

    return { botResponse, intent, confidence, entities };
  }
}

module.exports = RasaClient;
