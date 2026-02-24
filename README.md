# Bachelor Thesis - Voice-Controlled Smart Clock

This repository contains the implementation for my bachelor thesis on developing a voice-controlled smart clock system (SmUhr). The project combines embedded systems programming, voice processing, and natural language understanding to create an interactive device.

## Project Structure

The project consists of three main components:

### Device (`/device`)
ESP32-based smart clock with voice assistant capabilities. Built using ESP-IDF framework.

**Main features:**
- Wake word detection
- Audio streaming via WebSocket
- Display with custom UIs
- Alarm functionality
- WiFi connectivity

### API Backend (`/api`)
Node.js backend handling voice processing and system coordination.

**Components:**
- Voice processing pipeline (speech-to-text, NLP, text-to-speech)
- WebSocket server for device communication
- PostgreSQL database for data persistence
- Dashboard server for monitoring

### Dashboard (`/api/simple-dashboard`)
React-based web interface for monitoring and managing the system.

**Features:**
- Real-time data visualization/Session Tracking
- System status monitoring

## Supporting Technologies

### Rasa
Open-source NLP framework used for understanding user intents and managing conversational context. The system was set up from scratch, trained with example data, and configured with more intents than strictly needed for the study. This was necessary because initial tests with fewer examples resulted in low confidence scores, causing the system to fall back to the default intent too frequently.

### PostgreSQL
Relational database used primarily for session and conversation tracking throughout the study.

## Study Protocols

The `/protocols` directory contains 24 scanned study protocols from the bachelor thesis, uploaded for documentation. 

## Architecture Overview

The system uses a client-server architecture where the ESP32 device acts as the client, streaming audio to the backend API. The API processes the voice input through Whisper for speech recognition, routes it through Rasa for intent understanding, and generates audio responses using OpenAI's TTS. The audio is then streamed back to the device for playback.
