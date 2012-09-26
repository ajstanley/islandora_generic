<?php

/**
 * Creates fedora objects to represent folders and files from Dropbox folder
 * @global user $user
 * @global url $base_path
 * @param string $path
 * @return string
 */
function islandora_generic_dropbox_sync($path = null) {
  module_load_include('php', 'dropbox_client', 'dropbox');
  module_load_include('module', 'dropbox_client');

  global $user, $base_path;
  $results = array();
  //get client
  $dropbox = dropbox_client_get_client();
  $dbobj = $dropbox->account();
  $dirs_to_process = array();
  $dirs_to_process[] = array(
    'root' => variable_get('islandora_generic_collection_pid', 'islandora:root'),
    'content' => '/'
  );
  $dirs = array();
  $files = array();
  // convert each content object to array
  while (count($dirs_to_process) > 0) {
    $info = array_shift($dirs_to_process);
    $path = $info['content'];
    $metadata = $dropbox->metadata($path);
    $contents = $metadata->contents;
    if (!$contents) {
      continue;
    }

    //load array with results of current directory.  Each content object is either a dir or a file
    foreach ($contents as $content) {
      if ($content->is_dir) {
        $dir_contents = array(
          'root' => $content->path,
          'parent' => $info['root'],
          'modified' => $content->modified,
          'rev' => $content->rev
        );
        $dirs[] = $dir_contents;
        $dirs_to_process[] = array(
          'root' => $path,
          'content' => $content->path,
        );
      }
      else {
        $media = $dropbox->media($content->path);
        $file_contents = array(
          'path' => $content->path,
          'url' => $media->url,
          'modified' => $content->modified,
          'rev' => $content->rev,
          'mime_type' => $content->mime_type
        );
        if (!$media->url) {
          drupal_set_message($content->path . " has no url");
        }
        $files[] = $file_contents;
      }
    }
  }
  $marker = 'Stop here';
  $batch = array(
    'title' => "Creating Fedora objects from Dropbox contents",
    'progress_message' => t('Synced @current directores and files out of @total.'),
    'operations' => array(),
    'file' => drupal_get_path('module', 'islandora_generic') . '/islandora_generic_sync.inc',
  );

  foreach ($dirs as $dir) {
    $batch['operations'][] = array('islandora_generic_build_collection', array($dir));
  }
  foreach ($files as $file) {
    $batch['operations'][] = array('islandora_generic_build_object', array($file));
  }
  $collection_home = 'islandora/' . variable_get('islandora_generic_collection_pid', 'islandora:root');
  batch_set($batch);
  batch_process($collection_home);
}

/**
 * Builds Fedora collection object to represent Dropbox folder, and populates collection with objects representing Dropbox files
 * @param array $result
 */
function islandora_generic_build_collection($dir) {
  module_load_include('inc', 'islandora_generic', 'islandora_generic_database');
  $collection_path = $dir['root'];
  $parent_path = $dir['parent'];
  $collection_pid = islandora_generic_get_pid_from_path($collection_path);
  // build collection object if necessary
  if (!$collection_pid) {
    $path_parts = explode('/', $collection_path);
    $collection_name = array_pop($path_parts);
    if ($collection_name == '') {
      $collection_name = 'Dropbox Collection';
    }
    if ($parent_path == variable_get('islandora_generic_collection_pid', 'islandora:root')) {
      $parent_pid = $parent_path;
    }
    else {
      $parent_pid = islandora_generic_get_pid_from_path($parent_path);
    }
    $collection_pid = islandora_generic_build_collection_object($parent_pid, $collection_name, $collection_path, $dir);
  }
}

function islandora_generic_build_object($file) {
  module_load_include('inc', 'islandora', '/includes/islandora.ingest');
  module_load_include('inc', 'islandora_generic', 'islandora_generic_database');
  global $base_url;


  $object_data = islandora_generic_get_data_from_path($file['path']);

  if ($object_data && $object_data['revision'] == $file['rev']) {
    return;
  }
  if ($object_data && $object_data['revision'] != $file['rev']) {
    islandora_generic_update_object($file);

  }

  $collection = pathinfo($file['path'], PATHINFO_DIRNAME);
  $collection_pid = islandora_generic_get_pid_from_path($collection);
  $namespace = variable_get('islandora_generic_namespace', 'islandora');
  $filename = pathinfo($file['path'], PATHINFO_FILENAME);
  $thumbnail = $base_url . '/' . drupal_get_path('module', 'islandora_generic') . '/module_images/file_thumbnail.png';
  $content_models = array(array('pid' => 'islandora:genericCModel'));

  $fedora_object = islandora_ingest_get_object($content_models, $collection_pid, 'isMemberOf', $namespace);
  $fedora_object->label = $filename;

  $thumbnail_datastream = $fedora_object->constructDatastream('TN');
  $thumbnail_datastream->setContentFromUrl($thumbnail);
  $thumbnail_datastream->label = 'TN';
  $thumbnail_datastream->mimetype = 'image/png';
  $fedora_object->ingestDatastream($thumbnail_datastream);

  $source_datastream = $fedora_object->constructDatastream('SOURCE');
  $source_datastream->setContentFromString(file_get_contents($file['url']));
  $source_datastream->label = 'SOURCE';
  $source_datastream->mimetype = $file['mime_type'];
  $fedora_object->ingestDatastream($source_datastream);


  $new_fedora_object = islandora_ingest_add_object($fedora_object);
  $pid = $fedora_object->id;
  islandora_generic_set_pid_mapping($pid, $file['path'], $file['rev'], $file['modified']);
}

/**
 * Build collection object mapped to dropbox folder
 * @global url $base_url
 * @param pid $parent_pid
 * @param string $collection_name
 * @param string $path
 * @return pid
 */
function islandora_generic_build_collection_object($parent_pid, $collection_name, $path, $dir) {
  module_load_include('inc', 'islandora', '/includes/islandora.ingest');
  module_load_include('inc', 'islandora_generic', 'islandora_generic_database');
  global $base_url;
  $namespace = variable_get('islandora_generic_namespace', 'islandora');
  $thumbnail = $base_url . '/' . drupal_get_path('module', 'islandora_basic_collection') . '/module_images/Crystal_Clear_filesystem_folder_grey.png';

  $content_models = array(array('pid' => 'islandora:collectionCModel'));
  $fedora_object = islandora_ingest_get_object($content_models, $parent_pid, 'isMemberOf', $namespace);
  $fedora_object->label = $collection_name;

  $thumbnail_datastream = $fedora_object->constructDatastream('TN');
  $thumbnail_datastream->setContentFromUrl($thumbnail);
  $thumbnail_datastream->label = 'TN';
  $thumbnail_datastream->mimetype = 'image/png';
  $fedora_object->ingestDatastream($thumbnail_datastream);

  $new_fedora_object = islandora_ingest_add_object($fedora_object);
  $pid = $fedora_object->id;
  islandora_generic_set_pid_mapping($pid, $path, $dir['rev'], $dir['modified']);
  return $pid;
}

function islandora_generic_update_object($file) {
  $pid = $file['pid'];
  $fedora_object = islandora_object_load($pid);
  $fedora_object['SOURCE']->content = file_get_contents($file_url['url']);
  $fedora_object['SOURCE']->mimeType = $file('mime_type');
}
