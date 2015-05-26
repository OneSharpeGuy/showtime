SELECT a.title,
 a.start_time_unix,
 b.*,
 c.*,
 yes_count/yes_respondants*100 AS pct_yes,
 no_count/no_respondants*100 AS pct_no,
        limbo_count/limbo_respondants*100 AS pct_limbo,
        ((eligible_users-unregistered_count)/eligible_users) *100 as pct_responding
FROM osg_ical_imported a
INNER JOIN
 (
SELECT nid,
 voice_part, SUM(state_yes) AS yes_count, SUM(state_no) AS no_count, SUM(state_limbo) AS limbo_count, SUM(unregistered) AS unregistered_count
FROM osg_registration_ext
GROUP BY nid,
 voice_part WITH ROLLUP) b ON a.nid=b.nid
INNER JOIN
 (
SELECT nid, COUNT(DISTINCT uid) eligible_users, SUM(item_count) tot_respondants, SUM(state_yes) yes_respondants, SUM(state_no) no_respondants, SUM(state_limbo) limbo_respondants
FROM osg_registration_ext
GROUP BY nid) c ON a.nid=c.nid