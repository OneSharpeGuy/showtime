SELECT i.nid,
 u.uid,
 r.`count`,
 r.`state`, CASE r.`state` WHEN 'yes' THEN 1 ELSE 0 END AS state_yes
, CASE r.`state` WHEN 'no' THEN 1 ELSE 0 END AS state_no
, CASE r.`state` WHEN 'maybe' THEN 1 ELSE 0 END state_maybe
, CASE r.`state` WHEN 'probably_not' THEN 1 ELSE 0 END state_unlikely
FROM osg_ical_imported i
INNER JOIN osg_users u ON is_singer=1 AND i.available=1
LEFT JOIN registration r ON r.entity_id=i.nid AND u.uid=r.user_uid
ORDER BY nid,
 uid