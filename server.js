const express = require('express');
const jwt = require('jsonwebtoken');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = 3000;
const JWT_SECRET = 'smartlock-secret-key-2026';

// Credentials — stored server-side only
const ADMIN_USER = 'admin';
const ADMIN_PASS = 'admin123';

// Mobius config
const MOBIUS_IP = '172.20.10.6';
const MOBIUS_PORT = 7579;

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname)));

// Login endpoint
app.post('/api/login', (req, res) => {
  const { username, password } = req.body;
  if (username === ADMIN_USER && password === ADMIN_PASS) {
    const token = jwt.sign({ username }, JWT_SECRET, { expiresIn: '1h' });
    res.json({ token });
  } else {
    res.status(401).json({ error: 'Invalid credentials' });
  }
});

// Verify token middleware
function verifyToken(req, res, next) {
  const auth = req.headers['authorization'];
  if (!auth) return res.status(401).json({ error: 'No token' });
  const token = auth.split(' ')[1];
  try {
    req.user = jwt.verify(token, JWT_SECRET);
    next();
  } catch(e) {
    res.status(401).json({ error: 'Invalid or expired token' });
  }
}

// Proxy endpoints to Mobius
app.get('/api/status', verifyToken, async (req, res) => {
  try {
    const response = await fetch(`http://${MOBIUS_IP}:${MOBIUS_PORT}/Mobius/SmartLock/latest`, {
      headers: {
        'X-M2M-Origin': 'SmartLockAE',
        'X-M2M-RI': 'req-status',
        'X-M2M-RVI': '2a',
        'Accept': 'application/json'
      }
    });
    const data = await response.json();
    res.json(data);
  } catch(e) {
    res.status(500).json({ error: 'Mobius unreachable' });
  }
});

app.get('/api/log', verifyToken, async (req, res) => {
  try {
    const response = await fetch(`http://${MOBIUS_IP}:${MOBIUS_PORT}/Mobius/SmartLock?rcn=4&lim=20`, {
      headers: {
        'X-M2M-Origin': 'SmartLockAE',
        'X-M2M-RI': 'req-log',
        'X-M2M-RVI': '2a',
        'Accept': 'application/json'
      }
    });
    const data = await response.json();
    res.json(data);
  } catch(e) {
    res.status(500).json({ error: 'Mobius unreachable' });
  }
});

app.listen(PORT, () => {
  console.log(`SmartLock server running at http://localhost:${PORT}`);
});