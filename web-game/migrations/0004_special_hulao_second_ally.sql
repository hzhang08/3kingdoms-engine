-- Existing two-friendly matches keep their lineup and all recorded results.
ALTER TABLE special_hulao_matches ADD COLUMN ally2 INTEGER NOT NULL DEFAULT -1
 CHECK(ally2=-1 OR (enemy=119 AND ally2 IN (0,6,1,11) AND ally2<>hero AND ally2<>ally));
