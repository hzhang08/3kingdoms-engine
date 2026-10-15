-- Rebuild the roster constraints, retaining every existing match and result.
-- No tables reference special_hulao_matches. Wrangler applies this migration atomically.
CREATE TABLE special_hulao_matches_next (
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
 ally2 INTEGER NOT NULL DEFAULT -1,
 ally3 INTEGER NOT NULL DEFAULT -1,
 CHECK((enemy=119 AND ally IN (0,6,1,11) AND ally<>hero) OR
       (enemy=127 AND (ally=-1 OR (ally IN (0,6,1,11) AND ally<>hero)))),
 CHECK(ally2=-1 OR (enemy=119 AND ally2 IN (0,6,1,11) AND ally2<>hero AND ally2<>ally)),
 CHECK(ally3=-1 OR (enemy=119 AND ally2<>-1 AND ally3 IN (0,6,1,11) AND ally3<>hero AND ally3<>ally AND ally3<>ally2)),
 CHECK((outcome IS NULL AND finished_at IS NULL AND final_hash IS NULL AND final_turn IS NULL) OR
       (outcome IS NOT NULL AND finished_at IS NOT NULL AND final_hash IS NOT NULL AND final_turn BETWEEN 1 AND 20))
);
INSERT INTO special_hulao_matches_next
 (id,user_id,hero,enemy,ally,initial_hash,created_at,outcome,finished_at,final_hash,final_turn,ally2)
 SELECT id,user_id,hero,enemy,ally,initial_hash,created_at,outcome,finished_at,final_hash,final_turn,ally2 FROM special_hulao_matches;
DROP TABLE special_hulao_matches;
ALTER TABLE special_hulao_matches_next RENAME TO special_hulao_matches;
CREATE INDEX special_hulao_owner ON special_hulao_matches(user_id,created_at DESC);
CREATE INDEX special_hulao_results ON special_hulao_matches(user_id,outcome);
