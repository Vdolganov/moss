const fs = require('fs');
const path = require('path');
module.exports = function checkLaunchers() {
  for (const name of ['packaging/MOSS.sh', 'packaging/cpu-performance.sh']) {
    const bytes = fs.readFileSync(path.resolve(__dirname, '..', name));
    if (!bytes.subarray(0, 10).equals(Buffer.from('#!/bin/sh\n')) || bytes.includes(13)) {
      throw new Error(`${name}: Garlic launcher requires a plain UTF-8/ASCII file with LF line endings`);
    }
  }
};
if (require.main === module) module.exports();
