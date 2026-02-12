function log(level, component, message, data = null) {
  const text = `[${component}] ${message}`;
  const args = data ? [text, data] : [text];

  if (level === 'error') console.error(...args);
  else if (level === 'warn') console.warn(...args);
  else console.log(...args);
}

module.exports = {
  info: (component, message, data) => log('info', component, message, data),
  error: (component, message, data) => log('error', component, message, data),
  warn: (component, message, data) => log('warn', component, message, data),
  debug: (component, message, data) => {
    if (process.env.NODE_ENV === 'development') log('debug', component, message, data);
  }
};
