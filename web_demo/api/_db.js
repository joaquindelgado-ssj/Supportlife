const { neon } = require('@neondatabase/serverless');
let sql;
module.exports = function db() {
  if (!sql) sql = neon(process.env.DATABASE_URL);
  return sql;
};
