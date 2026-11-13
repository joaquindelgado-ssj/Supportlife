// GET /api/readings?device=ID&limit=60
const db = require('./_db');
module.exports = async (req, res) => {
  res.setHeader('Cache-Control', 'no-store');
  const device = String(req.query.device || 'brazalete-01').slice(0, 40);
  const limit = Math.min(Math.max(parseInt(req.query.limit || '60', 10) || 60, 1), 500);
  try {
    const sql = db();
    const rows = await sql`SELECT ts, bpm, spo2, temp_c, accel_x, accel_y, accel_z, motion, finger, bat_v, simulated
                           FROM readings WHERE device_id = ${device} ORDER BY ts DESC LIMIT ${limit}`;
    const devs = await sql`SELECT id, name, last_seen FROM devices ORDER BY last_seen DESC NULLS LAST`;
    const series = rows.reverse();
    res.status(200).json({ device, now: new Date().toISOString(), devices: devs,
                           latest: series.length ? series[series.length - 1] : null, series });
  } catch (e) {
    res.status(500).json({ error: 'error de base de datos' });
  }
};
