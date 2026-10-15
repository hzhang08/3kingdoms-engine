// Development-only test harness. Load alongside the generated prototype page.
// Uses public engine commands and the real IndexedDB save/load functions.
window.runPrototypeBrowserCheck = async function () {
  const call = (n, type, args = [], values = []) => Module.ccall(n, type, args, values);
  const state = () => JSON.parse(call('prototype_status', 'string'));
  const hash = () => call('prototype_hash', 'string');
  const command = (c, x = 0, y = 0) => {
    if (!call('prototype_command', 'number', ['string', 'number', 'number'], [c, x, y]))
      throw Error(call('prototype_error', 'string'));
  };
  const check = (condition, message) => { if (!condition) throw Error(message); };
  const frame = () => new Promise(resolve => requestAnimationFrame(resolve));
  const until = async predicate => {
    const deadline = performance.now() + 45000;
    while (!predicate()) { check(performance.now() < deadline, 'turn transition timed out'); await frame(); }
  };
  const commands = await (await fetch('prototype-test/commands.json')).json();
  command('restart');
  for (const step of commands) {
    await until(() => !call('prototype_busy', 'number') && state().phase === step.phase && state().turn === step.turn &&
      (step.command === 'continue') === Boolean(JSON.parse(call('prototype_dialogue', 'string'))));
    command(step.command, step.x, step.y);
    check(hash() === step.hash, `browser/native command divergence: ${JSON.stringify(step)}`);
    await frame();
  }
  await until(() => !call('prototype_busy', 'number'));
  check(state().phase === 'victory', 'victory was not reached');
  const victoryHash = hash();
  // Leave the actual victory frame visible for a screenshot before the next test.
  window.prototypeVerification = { victory: true, victoryHash, commands: commands.length };
  return window.prototypeVerification;
};

window.runPrototypePersistenceCheck = async function () {
  const call = (n, type, args = [], values = []) => Module.ccall(n, type, args, values);
  const check = (c, m) => { if (!c) throw Error(m); };
  const blob = await (await fetch('prototype-test/snapshot.json')).text();
  const next = await (await fetch('prototype-test/next.json')).json();
  const restore = b => call('prototype_restore', 'number', ['string'], [b]);
  const snapshot = () => call('prototype_snapshot', 'string');
  check(restore(blob) === 1, 'Linux snapshot import failed');
  await window.prototypeSave();
  call('prototype_command', 'number', ['string', 'number', 'number'], ['restart', 0, 0]);
  await window.prototypeLoad();
  check(snapshot() === blob, 'IndexedDB snapshot restoration differs');
  call('prototype_command', 'number', ['string', 'number', 'number'], [next.command, next.x, next.y]);
  check(call('prototype_hash', 'string') === next.hash, 'next action after IndexedDB reload differs');
  const db = await new Promise((resolve, reject) => {
    const r = indexedDB.open('caocao-opening-prototype', 1);
    r.onsuccess = () => resolve(r.result); r.onerror = () => reject(r.error);
  });
  const put = data => new Promise((resolve, reject) => {
    const tx = db.transaction('snapshots', 'readwrite');
    tx.objectStore('snapshots').put(data, 'slot-1'); tx.oncomplete = resolve; tx.onerror = () => reject(tx.error);
  });
  const get = () => new Promise((resolve, reject) => {
    const r = db.transaction('snapshots').objectStore('snapshots').get('slot-1');
    r.onsuccess = () => resolve(r.result); r.onerror = () => reject(r.error);
  });
  while (call('prototype_busy', 'number')) await new Promise(resolve => requestAnimationFrame(resolve));
  const before = snapshot();
  const damaged = blob.replace('"rng":', '"invalid_rng":');
  try {
    await put(damaged); await window.prototypeLoad();
    check(snapshot() === before, 'bad save changed the live battle');
    check(await get() === damaged, 'bad save was overwritten during rejection');
    check(document.querySelector('#status').textContent.includes('失败'), 'missing rejection feedback');
  } finally { await put(blob); db.close(); }
  await window.prototypeLoad();
  check(snapshot() === blob, 'older valid save no longer loads');
  window.prototypePersistenceVerification = { nextAttackHash: next.hash, corruptionRejected: true, olderSaveRestored: true };
  return window.prototypePersistenceVerification;
};

window.runPrototypeDefeatCheck = async function () {
  const call = (n, type, args = [], values = []) => Module.ccall(n, type, args, values);
  call('prototype_command', 'number', ['string', 'number', 'number'], ['restart', 0, 0]);
  const deadline = performance.now() + 300000;
  while (performance.now() < deadline) {
    const s = JSON.parse(call('prototype_status', 'string'));
    if (call('prototype_busy', 'number')) { await new Promise(resolve => requestAnimationFrame(resolve)); continue; }
    if (JSON.parse(call('prototype_dialogue', 'string'))) {
      call('prototype_command', 'number', ['string', 'number', 'number'], ['continue', 0, 0]);
      await new Promise(resolve => requestAnimationFrame(resolve)); continue;
    }
    if (s.phase === 'defeat') return { defeat: true, turn: s.turn, hash: call('prototype_hash', 'string') };
    if (s.phase === 'player') call('prototype_command', 'number', ['string', 'number', 'number'], ['end_turn', 0, 0]);
    await new Promise(resolve => requestAnimationFrame(resolve));
  }
  throw Error('defeat path timed out');
};
