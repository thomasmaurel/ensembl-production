-- Copyright [1999-2015] Wellcome Trust Sanger Institute and the EMBL-European Bioinformatics Institute
-- Copyright [2016-2017] EMBL-European Bioinformatics Institute
-- 
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
-- 
--      http://www.apache.org/licenses/LICENSE-2.0
-- 
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

# patch_91_92_e.sql
#
# Title: Removal of unused species, db tables.
#
# Description:
#   Remove tables used for tracking species etc.
DROP TABLE changelog;
DROP TABLE changelog_species;
DROP TABLE division_species;
DROP TABLE division_db;
DROP TABLE division;
DROP TABLE species_alias;
DROP TABLE species;
DROP VIEW db_list;
DROP TABLE db;


# Patch identifier
INSERT INTO meta (species_id, meta_key, meta_value)
  VALUES (NULL, 'patch', 'patch_91_92_e.sql|remove_species');