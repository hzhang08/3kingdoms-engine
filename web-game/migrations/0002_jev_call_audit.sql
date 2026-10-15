ALTER TABLE jev_decisions ADD COLUMN provider_attempted INTEGER NOT NULL DEFAULT 0;
ALTER TABLE jev_decisions ADD COLUMN provider_status INTEGER;
ALTER TABLE jev_decisions ADD COLUMN outcome TEXT NOT NULL DEFAULT 'not_called';
