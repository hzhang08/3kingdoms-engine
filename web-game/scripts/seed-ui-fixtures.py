"""Seed only the isolated local UI database with C++-validated victory fixtures.

Never used by production builds or remote D1. Slots 6–9 must be empty.
"""
import json, pathlib, sqlite3, time, uuid
root = pathlib.Path(__file__).resolve().parents[1]
config = json.loads((root / 'wrangler.ui-test.json').read_text())
assert config['name'] == 'caocao-local-ui-only'
assert config['vars']['APP_ORIGIN'] == 'http://127.0.0.1:8788'
databases = [p for p in (root / '.wrangler/ui-state/v3/d1').rglob('*.sqlite') if p.name != 'metadata.sqlite']
assert len(databases) == 1, 'Expected exactly one isolated UI database'
with sqlite3.connect(databases[0]) as db:
    assert db.execute("SELECT id FROM user WHERE id='local-ui-only'").fetchone()
    assert not db.execute("SELECT slot FROM saves WHERE user_id='local-ui-only' AND slot BETWEEN 6 AND 9").fetchall(), 'Fixture slots already populated; no overwrite performed'
    for scenario in range(4):
        raw = (root.parent / f'docs/evidence/web-game/campaign-fixtures/{scenario}-victory.json').read_text()
        envelope = json.loads(raw); p = envelope['payload']; now = int(time.time()*1000); run = str(uuid.uuid4())
        assert p['scenario'] == scenario and p['schema'] == 10 and p['state']['phase'] == 'victory'
        db.execute('INSERT INTO runs(id,user_id,seed,created_at,updated_at,cleared) VALUES(?,?,?,?,?,?)', (run,'local-ui-only',1,now,now,scenario))
        db.execute('INSERT INTO saves(user_id,slot,run_id,snapshot,state_hash,scenario,turn,phase,cleared,revision,saved_at) VALUES(?,?,?,?,?,?,?,?,?,1,?)', ('local-ui-only',scenario+6,run,raw,envelope['sha256'],scenario,p['state']['turn'],'victory',scenario+1,now))
print('Seeded synthetic victory fixtures in local UI slots 6–9; production untouched.')
