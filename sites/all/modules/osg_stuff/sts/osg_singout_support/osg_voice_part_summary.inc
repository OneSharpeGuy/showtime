<?php

function osg_singout_support_osg_voicepart_summary($key_fields = array()) {
  //debug($key_fields);
  /*
  SELECT i.nid, d.tid,d.name,d.weight
  FROM taxonomy_vocabulary v
  inner join taxonomy_term_data d on v.vid=d.vid
  inner join osg_ical_imported i on i.available=1
  where v.machine_name='voice_part' and d.name<>'N/A'

  */
  $sub_query = db_select('taxonomy_vocabulary','v');
  $sub_query->join('taxonomy_term_data','d','v.vid=d.vid');
  $sub_query->join('osg_ical_imported','i','i.available=1');
  $sub_query->leftJoin('osg_voice_part_summary','s','s.nid=i.nid AND d.tid=s.tid');
  $sub_query->fields('i',array('nid'));
  $sub_query->fields('d',array('tid','name','weight'));
  $sub_query->isNull('s.ovps_id');
  $sub_query->condition('d.name','N/A','<>');
  if (isset($key_fields['nid'])) {
    $sub_query->condition('i.nid',$key_fields['nid']);
  }
  $sub_query->where('v.machine_name=:voice_part',array(':voice_part'=>'voice_part'));

  $query = db_insert('osg_voice_part_summary');
  $query->fields(array('nid','tid','voice_part','weight'));
  $query->from($sub_query);

  $result     = $query->execute();
  /*
  SELECT nid,
  NULL,
  'Total',
  999
  FROM osg_ical_imported
  WHERE nid NOT IN
  (SELECT DISTINCT nid
  FROM osg_voice_part_summary
  WHERE tid IS NULL)
  AND nid IN
  (SELECT DISTINCT nid
  FROM osg_voice_part_summary)
  */
  $sub_query1 = db_select('osg_voice_part_summary','s');
  $sub_query1->distinct();
  $sub_query1->fields('s',array('nid'));

  $sub_query2 = db_select('osg_voice_part_summary','s');
  $sub_query2->distinct();
  $sub_query2->fields('s',array('nid'));
  $sub_query2->isNull('tid');

  $sub_query = db_select('osg_ical_imported','i');
  $sub_query->condition('nid',$sub_query1,'IN');
  $sub_query->condition('nid',$sub_query2,'NOT IN');
  $sub_query->fields('i',array('nid'));
  $sub_query->addExpression('NULL');
  $sub_query->addExpression(':total','voice_part',array(':total'=>'Total'));
  $sub_query->addExpression(999);
  if (isset($key_fields['nid'])) {
    $sub_query->condition('i.nid',$key_fields['nid']);
  }

  $query = db_insert('osg_voice_part_summary');
  $query->fields(array('nid','tid','voice_part','weight'));
  $query->from($sub_query);

  $result     = $query->execute();
  /* The actual update is going to have to consist of stages.

  First stage, get the summary row set.
  The loop through the set updating a row at a time.  (Yeah, I know, that aint sql)
  */

  /*
  SELECT nid, voice_part,
  SUM(state_yes) AS yes_count ,
  SUM(state_no) AS no_count ,
  SUM(state_maybe) AS maybe_count ,
  SUM(state_unlikely) AS unlikely_count ,
  SUM(unregistered) AS unregistered_count ,
  SUM(item_count) AS responding ,
  COUNT(DISTINCT uid) user_count
  FROM osg_registration_ext
  GROUP BY nid,voice_part
  */
  /*
  SELECT COUNT( * )
  FROM osg_users
  WHERE is_singer = 1
  AND is_member=1
  */
  $query      = db_select('osg_users');
  $query->condition('is_singer',1);
  $query->condition('is_member',1);
  $query->addExpression('COUNT(*)');
  $user_count = $query->execute()->fetchField();

  $query      = db_select('osg_registration_ext','x');
  $query->join('taxonomy_term_data','d','x.voice_part=d.name');
  $query->groupBy('x.nid');
  $query->groupBy('d.tid');
  $query->fields('x',array('nid'));
  $query->fields('d',array('tid'));
  $query->addExpression('SUM(state_yes)','yes_count');
  $query->addExpression('SUM(state_no)','no_count');
  $query->addExpression('SUM(state_maybe)','maybe_count');
  $query->addExpression('SUM(state_unlikely)','unlikely_count');
  $query->addExpression('SUM(unregistered)','unregistered_count');
  $query->addExpression('SUM(item_count)','responding');
  $query->addExpression('COUNT(DISTINCT x.uid)','user_count');
  if (isset($key_fields['nid'])) {
    $query->condition('x.nid',$key_fields['nid']);
  }

  //debug((string)$query);
  $result = $query->execute();
  while ($record = $result->fetchAssoc()) {
    //debug($record);
    $fields = array();
    foreach ($record as $key => $value) {
      $fields[$key] = $value;
    }

    $query = db_update('osg_voice_part_summary');
    $query->fields($fields);
    $query->condition('tid',$record['tid']);
    $query->condition('nid',$record['nid']);
    //debug((string)$query);
    //debug(array('tid'=>$record['tid'],'nid'=>$record['nid']));
    $query->execute();

  }
  $query = db_select('osg_registration_ext','x');
  $query->groupBy('x.nid');
  $query->fields('x',array('nid'));
  $query->addExpression('SUM(state_yes)','yes_count');
  $query->addExpression('SUM(state_no)','no_count');
  $query->addExpression('SUM(state_maybe)','maybe_count');
  $query->addExpression('SUM(state_unlikely)','unlikely_count');
  $query->addExpression('SUM(unregistered)','unregistered_count');
  $query->addExpression('SUM(item_count)','responding');
  $query->addExpression('COUNT(DISTINCT x.uid)','user_count');
  if (isset($key_fields['nid'])) {
    $query->condition('x.nid',$key_fields['nid']);
  }

  //debug((string)$query);
  $result = $query->execute();
  while ($record = $result->fetchAssoc()) {
    //debug($record);
    $fields = array();
    foreach ($record as $key => $value) {
      $fields[$key] = $value;
    }

    $query = db_update('osg_voice_part_summary');
    $query->fields($fields);
    $query->isNull('tid');
    $query->condition('nid',$record['nid']);
    //debug((string)$query);
    //debug(array('tid'=>$record['tid'],'nid'=>$record['nid']));
    $query->execute();

    //Total Responding
    $query = db_update('osg_voice_part_summary');
    $query->fields(array('total_responding'=>$record['responding']));    
    $query->condition('nid',$record['nid']);
    $query->execute();


    //UPDATE osg_voice_part_summary
    //SET user_count =
    //  (SELECT COUNT( * )
    //   FROM osg_users
    //   WHERE is_singer = 1
    //     AND is_member = 1)

    $query = db_update('osg_voice_part_summary');
    $query->fields(array('user_count'=>$user_count));
    $query->condition('nid',$record['nid']);
    $query->execute();
  }
}
