FROM node:20-alpine AS builder

WORKDIR /app

COPY package*.json ./
RUN npm ci

COPY . .

ARG VITE_API_BASE_URL
ARG VITE_API_TOKEN

ENV VITE_API_BASE_URL=${VITE_API_BASE_URL}
ENV VITE_API_TOKEN=${VITE_API_TOKEN}

RUN npm run build

FROM node:20-alpine

WORKDIR /app

COPY --from=builder /app/dist ./dist

RUN npm install --production express

RUN echo 'const express = require("express"); \
const path = require("path"); \
const app = express(); \
app.use(express.static(path.join(__dirname, "dist"))); \
app.use((req, res) => { \
  res.sendFile(path.join(__dirname, "dist", "index.html")); \
}); \
const PORT = process.env.PORT || 3000; \
app.listen(PORT, () => console.log(`Dashboard running on port ${PORT}`));' > server.cjs

EXPOSE 3000

CMD ["node", "server.cjs"]
