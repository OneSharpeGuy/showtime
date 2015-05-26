SELECT b.*,
       a.user_count,
       sum_yes/user_count*100 AS pct_yes
       , sum_no/user_count*100 as pct_no
       , (sum_maybe+sum_unlikely)/user_count*100 as pct_limbo
       , sum_unregistered/user_count*100 as pct_unregistered
       , (voice_part_count-sum_unregistered)/voice_part_count*100 as pct_responding
FROM
  (SELECT x.nid ,
          x.voice_part ,
          SUM(state_yes) sum_yes ,
          SUM(state_no) sum_no ,
          SUM(state_maybe) sum_maybe ,
          SUM(state_unlikely) sum_unlikely ,
          SUM(unregistered) sum_unregistered ,
          COUNT(*) voice_part_count
   FROM osg_registration_ext x
   INNER JOIN osg_users u ON x.uid = u.uid
   AND u.is_member=1
   AND u.is_singer=1
   GROUP BY nid,
            voice_part WITH ROLLUP) b
INNER JOIN
  (SELECT nid,
          count(*) AS user_count
   FROM osg_registration_ext x
   INNER JOIN osg_users u ON x.uid = u.uid
   AND u.is_member=1
   AND u.is_singer=1
   GROUP BY nid) a ON b.nid=a.nid
   LEFT join taxonomy_term_data d on d.name=voice_part
   order by nid,coalesce(d.weight,999)