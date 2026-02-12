function validateWebSocketAuth(request) {
  const authHeader = request.headers.authorization;

  if (!authHeader) {
    return { valid: false, error: 'Missing Authorization header', code: 401 };
  }

  const token = authHeader.startsWith('Bearer ') ? authHeader.slice(7) : authHeader;

  if (token !== process.env.API_TOKEN) {
    return { valid: false, error: 'Invalid token', code: 403 };
  }

  return { valid: true };
}

function validateApiAuth(req, res, next) {
  const authHeader = req.headers.authorization;
  const queryToken = req.query.token;

  let token = null;
  if (authHeader) {
    token = authHeader.startsWith('Bearer ') ? authHeader.slice(7) : authHeader;
  } else if (queryToken) {
    token = queryToken;
  }

  if (!token) {
    return res.status(401).json({ success: false, error: 'Missing Authorization' });
  }

  if (token !== process.env.API_TOKEN) {
    return res.status(403).json({ success: false, error: 'Invalid token' });
  }

  next();
}

module.exports = { validateWebSocketAuth, validateApiAuth };
