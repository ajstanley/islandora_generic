<?php

/**
 * Pesrsists pid to path mapping in database
 * @param pid $pid
 * @param string $path
 */
function islandora_generic_set_pid_mapping($pid, $path, $rev, $modified) {
  $object = new stdClass();
  $object->PID = $pid;
  $object->dropbox_path = $path;
  $object->modified = $modified;
  $object->revision = $rev;
  $result = drupal_write_record('islandora_generic_mappings', $object);
}

/**
 * Returns PID associated with object representing dropbox object.
 * @param string $path
 * @return PID
 */
function islandora_generic_get_pid_from_path($path) {
  //get pid if it exists
  $collection_pid = db_query('SELECT PID 
    FROM {islandora_generic_mappings}
    WHERE dropbox_path = :path', array(':path' => $path))->fetchField();
  return $collection_pid;
}

/**
 * Returns PID and revision associated with object representing dropbox object.
 * @param string $path
 * @return PID
 */
function islandora_generic_get_data_from_path($path) {


  $result = db_query('SELECT n.pid, n.revision
FROM {islandora_generic_mappings} n
WHERE n.dropbox_path = :path', array(':path' => $path))->fetchAssoc();

  return $result;
}