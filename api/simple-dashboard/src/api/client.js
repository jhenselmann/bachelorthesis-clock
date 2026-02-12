import axios from 'axios';

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || '/api';
const API_TOKEN = import.meta.env.VITE_API_TOKEN || '';

const api = axios.create({
  baseURL: API_BASE_URL,
  headers: {
    'Content-Type': 'application/json',
    'Authorization': `Bearer ${API_TOKEN}`
  }
});

api.interceptors.response.use(
  res => res.data,
  err => { throw err; }
);

export const sessionsApi = {
  getAll: (params = {}) => api.get('/sessions', { params }),
  getInteractions: (sessionId) => api.get(`/sessions/${sessionId}/interactions`),
  getAudioUrl: (sessionId) => `${API_BASE_URL}/sessions/${sessionId}/audio?token=${API_TOKEN}`
};
