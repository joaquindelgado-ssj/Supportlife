// POST /api/ingest  -> lo usa el ESP32 (header x-api-key obligatorio)
const db = require('./_db');
const { clean, insert } = require('./_util');
module.exports = async (req, res) => {
  if (req.method !== 'POST') return res.status(405).json({ error: 'usar POST' });
  const key = process.env.INGEST_KEY;
  if (!key || req.headers['x-api-key'] !== key) return res.status(401).json({ error: 'api key invalida' });
  try {
    await insert(db(), clean(req.body), false);
    res.status(201).json({ ok: true });
  } catch (e) {
    res.status(500).json({ error: 'error de base de datos' });
  }
};
