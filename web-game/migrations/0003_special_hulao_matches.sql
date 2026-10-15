CREATE TABLE special_hulao_matches (
 id TEXT PRIMARY KEY,
 user_id TEXT NOT NULL REFERENCES user(id) ON DELETE CASCADE,
 hero INTEGER NOT NULL CHECK(hero IN (0,6,1,11)),
 enemy INTEGER NOT NULL CHECK(enemy IN (119,127)),
 ally INTEGER NOT NULL,
 initial_hash TEXT NOT NULL,
 created_at INTEGER NOT NULL,
 outcome TEXT CHECK(outcome IN ('victory','defeat')),
 finished_at INTEGER,
 final_hash TEXT,
 final_turn INTEGER,
 CHECK((enemy=119 AND ally IN (0,6,1,11) AND ally<>hero) OR (enemy=127 AND ally=-1)),
 CHECK((outcome IS NULL AND finished_at IS NULL AND final_hash IS NULL AND final_turn IS NULL) OR
       (outcome IS NOT NULL AND finished_at IS NOT NULL AND final_hash IS NOT NULL AND final_turn BETWEEN 1 AND 20))
);
CREATE INDEX special_hulao_owner ON special_hulao_matches(user_id,created_at DESC);
CREATE INDEX special_hulao_results ON special_hulao_matches(user_id,outcome);
