CREATE TABLE user (id TEXT PRIMARY KEY, name TEXT NOT NULL, email TEXT NOT NULL UNIQUE, emailVerified INTEGER NOT NULL DEFAULT 0, image TEXT, createdAt INTEGER NOT NULL, updatedAt INTEGER NOT NULL);
CREATE TABLE session (id TEXT PRIMARY KEY, expiresAt INTEGER NOT NULL, token TEXT NOT NULL UNIQUE, createdAt INTEGER NOT NULL, updatedAt INTEGER NOT NULL, ipAddress TEXT, userAgent TEXT, userId TEXT NOT NULL REFERENCES user(id) ON DELETE CASCADE);
CREATE INDEX session_user ON session(userId);
CREATE TABLE account (id TEXT PRIMARY KEY, accountId TEXT NOT NULL, providerId TEXT NOT NULL, userId TEXT NOT NULL REFERENCES user(id) ON DELETE CASCADE, accessToken TEXT, refreshToken TEXT, idToken TEXT, accessTokenExpiresAt INTEGER, refreshTokenExpiresAt INTEGER, scope TEXT, password TEXT, createdAt INTEGER NOT NULL, updatedAt INTEGER NOT NULL);
CREATE INDEX account_user ON account(userId);
CREATE UNIQUE INDEX account_provider ON account(providerId, accountId);
CREATE TABLE verification (id TEXT PRIMARY KEY, identifier TEXT NOT NULL, value TEXT NOT NULL, expiresAt INTEGER NOT NULL, createdAt INTEGER, updatedAt INTEGER);
CREATE TABLE rateLimit (id TEXT PRIMARY KEY, key TEXT NOT NULL UNIQUE, count INTEGER NOT NULL, lastRequest INTEGER NOT NULL);
CREATE TABLE profiles (user_id TEXT PRIMARY KEY REFERENCES user(id) ON DELETE CASCADE, username TEXT NOT NULL, username_key TEXT NOT NULL UNIQUE, created_at INTEGER NOT NULL);
CREATE TABLE runs (
 id TEXT PRIMARY KEY, user_id TEXT NOT NULL REFERENCES user(id) ON DELETE CASCADE,
 seed INTEGER NOT NULL, created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL,
 cleared INTEGER NOT NULL DEFAULT 0 CHECK(cleared BETWEEN 0 AND 4),
 scenario INTEGER NOT NULL DEFAULT 0 CHECK(scenario BETWEEN 0 AND 3),
 battle_progress INTEGER NOT NULL DEFAULT 0 CHECK(battle_progress BETWEEN 0 AND 1000),
 turn INTEGER NOT NULL DEFAULT 2, phase TEXT NOT NULL DEFAULT 'player',
 completed_at INTEGER, checkpoint TEXT, revision INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX runs_owner ON runs(user_id, updated_at DESC);
CREATE INDEX runs_ranking ON runs(cleared DESC,battle_progress DESC,updated_at);
CREATE INDEX runs_completed ON runs(completed_at);
CREATE TABLE saves (
 user_id TEXT NOT NULL REFERENCES user(id) ON DELETE CASCADE, slot INTEGER NOT NULL CHECK(slot BETWEEN 0 AND 10),
 run_id TEXT NOT NULL REFERENCES runs(id), snapshot TEXT NOT NULL, state_hash TEXT NOT NULL,
 scenario INTEGER NOT NULL, turn INTEGER NOT NULL, phase TEXT NOT NULL, cleared INTEGER NOT NULL,
 revision INTEGER NOT NULL DEFAULT 1, saved_at INTEGER NOT NULL,
 PRIMARY KEY(user_id,slot)
);
CREATE INDEX saves_run ON saves(run_id);
CREATE TABLE budgets (bucket TEXT NOT NULL, window INTEGER NOT NULL, used INTEGER NOT NULL, PRIMARY KEY(bucket,window));
CREATE TABLE jev_decisions (
 user_id TEXT NOT NULL REFERENCES user(id) ON DELETE CASCADE, request_key TEXT NOT NULL,
 run_id TEXT NOT NULL REFERENCES runs(id), choice TEXT NOT NULL, source TEXT NOT NULL,
 created_at INTEGER NOT NULL, elapsed_ms INTEGER NOT NULL DEFAULT 0,
 PRIMARY KEY(user_id,request_key)
);
CREATE INDEX jev_date ON jev_decisions(created_at);
