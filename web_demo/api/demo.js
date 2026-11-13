// POST /api/demo -> lo usa el simulador del navegador (sin clave, queda marcado simulated=true)
const db = require('./_db');
const { clean, insert } = require('./_util');
module.exports = async (req, res) => {
  if (req.method !== 'POST') return res.status(405).json({ error: 'usar POST' });
  try {
    const r = clean(req.body);
    r.device = 'demo-simulador';
    await insert(db(), r, true);
    res.status(201).json({ ok: true });
  } catch (e) {
    res.status(500).json({ error: 'error de base de datos' });
  }
};
