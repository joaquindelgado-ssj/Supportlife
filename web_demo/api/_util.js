const num = (v, lo, hi) => {
  if (v === null || v === undefined || v === '') return null;
  const n = Number(v);
  if (!Number.isFinite(n) || n < lo || n > hi) return null;
  return n;
};
function clean(b) {
  b = b || {};
  return {
    device: String(b.device || 'brazalete-01').slice(0, 40).replace(/[^\w.-]/g, '') || 'brazalete-01',
    bpm: num(b.bpm, 20, 250),
    spo2: num(b.spo2, 50, 100),
    temp: num(b.temp, 20, 45),
    ax: num(b.ax, -50, 50), ay: num(b.ay, -50, 50), az: num(b.az, -50, 50),
    motion: num(b.motion, 0, 100),
    finger: typeof b.finger === 'boolean' ? b.finger : null,
    bat: num(b.bat, 2, 5),
  };
}
async function insert(sql, r, simulated) {
  await sql`INSERT INTO devices (id, name, last_seen) VALUES (${r.device}, ${r.device}, now())
            ON CONFLICT (id) DO UPDATE SET last_seen = now()`;
  await sql`INSERT INTO readings (device_id, bpm, spo2, temp_c, accel_x, accel_y, accel_z, motion, finger, bat_v, simulated)
            VALUES (${r.device}, ${r.bpm}, ${r.spo2}, ${r.temp}, ${r.ax}, ${r.ay}, ${r.az}, ${r.motion}, ${r.finger}, ${r.bat}, ${simulated})`;
}
module.exports = { clean, insert };
