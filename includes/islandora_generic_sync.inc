<?php

/**
 * Creates fedora objects to represent folders and files from Dropbox folder
 * @global user $user
 * @global url $base_path
 * @param string $path
 * @return string
 */
define('HAS_DB_PATH', 'hasDBpath');
define('HAS_DB_REV', 'hasDBrev');
define('IS_DECENDANT_OF', 'isDecendentOf');
define('HAS_DB_TOKEN', 'hasDBToken');
define('IS_DB_ORPHAN', 'isDBOrphan');

function islandora_generic_dropbox_sync($path = null) {
  module_load_include('php', 'dropbox_client', 'dropbox');
  module_load_include('module', 'dropbox_client');
  $islandora_generic_basepath = variable_get('islandora_generic_base_dir', '/');
  if ($islandora_generic_basepath[0] != '/') {
    $islandora_generic_basepath = "/$islandora_generic_basepath";
  }
  global $user, $base_path;
  $results = array();
//get client
  $dropbox = dropbox_client_get_client();
  $dbobj = $dropbox->account();
  $dirs_to_process[] = array(
    'parent_path' => $islandora_generic_basepath,
    'dir_path' => $islandora_generic_basepath
  );
  $dirs = array();
  $files = array();
// convert each content object to array
  while (count($dirs_to_process) > 0) {
    $info = array_shift($dirs_to_process);
    $path = $info['dir_path'];
    $metadata = $dropbox->metadata($path);
    if ($metadata && $metadata->contents) {
      $contents = $metadata->contents;
    }
    else {
      continue;
    }

//load array with results of current directory.  Each content object is either a dir or a file
    foreach ($contents as $content) {
      if ($content->is_dir) {
        $dir_contents = array(
          'path' => $content->path,
          'parent' => $info['parent_path'],
          'modified' => $content->modified,
          'rev' => $content->rev
        );
        $dirs[] = $dir_contents;
        $dirs_to_process[] = array(
          'parent_path' => $path,
          'dir_path' => $content->path,
        );
      }
      else {
        $media = $dropbox->media($content->path);
        if (isset($media->error)) {
          drupal_set_message($content->path . " has error: " . $media->error);
          continue;
        }
        $file_contents = array(
          'parent' => $info['dir_path'],
          'path' => $content->path,
          'url' => $media->url,
          'modified' => $content->modified,
          'rev' => $content->rev,
          'mime_type' => $content->mime_type
        );

        $files[] = $file_contents;
      }
    }
  }

  $batch = array(
    'title' => "Creating Fedora objects from Dropbox contents",
    'progress_message' => t('Synced @current directories and files out of @total.'),
    'operations' => array(),
    'file' => drupal_get_path('module', 'islandora_generic') . '/includes/islandora_generic_sync.inc',
  );

  foreach ($dirs as $dir) {
    $batch['operations'][] = array('islandora_generic_build_collection', array($dir));
  }
  foreach ($files as $file) {
    $batch['operations'][] = array('islandora_generic_build_object', array($file));
  }
  $collection_home = 'islandora/object/' . variable_get('islandora_generic_collection_pid', 'islandora:root');
  batch_set($batch);
  batch_process($collection_home);
}

/**
 * Builds Fedora collection object to represent Dropbox folder, and populates collection with objects representing Dropbox files
 * @param array $result
 */
function islandora_generic_build_collection($dir) {
  module_load_include('inc', 'islandora_generic', 'includes/islandora_generic_database');
  module_load_include('inc', 'islandora', 'includes/tuque');

  $ancestor = variable_get('islandora_generic_collection_pid', 'islandora:root');
  $token = variable_get('dropbox_client_website_oauth_token', '');
  $collection_path = $dir['path'];
  $repository = new IslandoraTuque();
  $query = "select \$pid from <#ri>
            where \$pid <http://islandora.ca/ontology/relsext#hasDBpath> '$collection_path'
            and \$pid <http://islandora.ca/ontology/relsext#hasDBToken> '$token'
            and \$pid <http://islandora.ca/ontology/relsext#isDecendentOf> '$ancestor'";

  $response = $repository->repository->ri->itqlQuery($query);
  $collection_pid = $response[0]['pid']['value'];

// build collection object if necessary
  if (!$collection_pid) {
    $path_parts = explode('/', $collection_path);
    $collection_name = array_pop($path_parts);
    if ($collection_name == '') {
      $collection_name = 'Dropbox Collection';
    }

    $parent_path = $dir['parent'];
    if ($parent_path == variable_get('islandora_generic_collection_pid', 'islandora:root') || $parent_path == '/') {
      $parent_pid = variable_get('islandora_generic_collection_pid', 'islandora:root');
    }
    else {
      $query = "select \$pid from <#ri>
            where \$pid <http://islandora.ca/ontology/relsext#hasDBpath> '$parent_path'
            and \$pid <http://islandora.ca/ontology/relsext#hasDBToken> '$token'
            and \$pid <http://islandora.ca/ontology/relsext#isDecendentOf> '$ancestor'";

      $response = $repository->repository->ri->itqlQuery($query);
      $parent_pid = $response[0]['pid']['value'];
    }
    $collection_pid = islandora_generic_build_collection_object($parent_pid, $collection_name, $collection_path, $dir);
  }
}

function islandora_generic_build_object($file) {
  module_load_include('inc', 'islandora', '/includes/ingest.form');
  module_load_include('inc', 'islandora_generic', 'islandora_generic_database');
  module_load_include('inc', 'islandora', 'includes/tuque');
  global $base_url;

  $islandora_generic_basepath = variable_get('islandora_generic_base_dir', '/');
  if($islandora_generic_basepath[0] != '/'){
    $islandora_generic_basepath = "/$islandora_generic_basepath";
  }
  $ancestor = variable_get('islandora_generic_collection_pid', 'islandora:root');
  $token = variable_get('dropbox_client_website_oauth_token', '');


  $file_path = $file['path'];
  $repository = new IslandoraTuque();
  $query = "select \$pid from <#ri>
            where \$pid <http://islandora.ca/ontology/relsext#hasDBpath> '$file_path'
            and \$pid <http://islandora.ca/ontology/relsext#hasDBToken> '$token'
            and \$pid <http://islandora.ca/ontology/relsext#isDecendentOf> '$ancestor'";
  $response = $repository->repository->ri->itqlQuery($query);
  $pid = $response[0]['pid']['value'];
  if ($pid) {
    $query = "select \$rev from <#ri>
            where <info:fedora/$pid> <http://islandora.ca/ontology/relsext#hasDBrev> \$rev";
    $response = $repository->repository->ri->itqlQuery($query);
    $rev = $response[0]['rev']['value'];
    if ($rev != $file['rev']) {
      islandora_generic_update_object($file);
    }
    return;
  }

  $parent_path = $file['parent'];
  if ($parent_path == $islandora_generic_basepath) {
    $collection_pid = variable_get('islandora_generic_collection_pid', 'islandora:root');
  }
  else {
    $query = "select \$pid from <#ri>
            where \$pid <http://islandora.ca/ontology/relsext#hasDBpath> '$parent_path'
            and \$pid <http://islandora.ca/ontology/relsext#hasDBToken> '$token'
            and \$pid <http://islandora.ca/ontology/relsext#isDecendentOf> '$ancestor'";

    $response = $repository->repository->ri->itqlQuery($query);
    $collection_pid = $response[0]['pid']['value'];
  }

  $namespace = variable_get('islandora_generic_namespace', 'islandora');
  $file_name = pathinfo($file['path'], PATHINFO_BASENAME);
  $thumbnail = $base_url . '/' . drupal_get_path('module', 'islandora_generic') . '/module_images/file_thumbnail.png';
  $config = array(
    'namespace' => $namespace,
    'label' => $file_name,
    'collections' => array($collection_pid),
    'models' => array('islandora:collectionCModel')
  );
  $fedora_object = islandora_ingest_form_prepare_new_object($config);
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

  $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, HAS_DB_PATH, $file['path'], TRUE);
  $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, HAS_DB_REV, $file['rev'], TRUE);
  $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, IS_DECENDANT_OF, $ancestor, TRUE);
  $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, HAS_DB_TOKEN, $token, TRUE);


  $new_fedora_object = islandora_add_object($fedora_object);
  $pid = $fedora_object->id;
// islandora_generic_set_pid_mapping($pid, $file['path'], $file['rev'], $file['modified']);
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
  module_load_include('inc', 'islandora', '/includes/ingest.form');
  module_load_include('inc', 'islandora_generic', 'islandora_generic_database');
  global $base_url;
  $ancestor = variable_get('islandora_generic_collection_pid', 'islandora:root');
  $token = variable_get('dropbox_client_website_oauth_token', '');
  $namespace = variable_get('islandora_generic_namespace', 'islandora');
  $thumbnail = $base_url . '/' . drupal_get_path('module', 'islandora_generic') . '/module_images/Crystal_Clear_filesystem_folder_grey.png';
  $config = array(
    'namespace' => $namespace,
    'label' => $collection_name,
    'collections' => array($parent_pid),
    'models' => array('islandora:collectionCModel')
  );
  $fedora_object = islandora_ingest_form_prepare_new_object($config);
  $thumbnail_datastream = $fedora_object->constructDatastream('TN');
  $thumbnail_datastream->setContentFromUrl($thumbnail);
  $thumbnail_datastream->label = 'TN';
  $thumbnail_datastream->mimetype = 'image/png';
  $fedora_object->ingestDatastream($thumbnail_datastream);

  $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, HAS_DB_PATH, $dir['path'], TRUE);
  $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, HAS_DB_REV, $dir['rev'], TRUE);
  $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, IS_DECENDANT_OF, $ancestor, TRUE);
  $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, HAS_DB_TOKEN, $token, TRUE);
  $new_fedora_object = islandora_add_object($fedora_object);
  $pid = $fedora_object->id;
  return $pid;
}

function islandora_generic_update_object($file) {
  $pid = $file['pid'];
  $fedora_object = islandora_object_load($pid);
  $fedora_object['SOURCE']->content = file_get_contents($file_url['url']);
  $fedora_object['SOURCE']->mimeType = $file['mime_type'];
}

/**
 * Finds all Objects decending from Islandor Generic root collection
 * which no longer have corresponding files in registered dropbox
 * @global user $user
 * @global url $base_path
 * @return string
 *  returned string is a themed list of orphaned pids
 */
function islandora_generic_find_orphans() {
  module_load_include('php', 'dropbox_client', 'dropbox');
  module_load_include('module', 'dropbox_client');
  module_load_include('inc', 'islandora', 'includes/tuque');

  global $user, $base_path;
  $results = array();
//get client
  $dropbox = dropbox_client_get_client();
  $dbobj = $dropbox->account();
// initialize directories and db paths
  $root_pid = variable_get('islandora_generic_collection_pid', '');
  $islandora_generic_basepath = variable_get('islandora_generic_base_dir', '/');
  $dirs_to_process = array($islandora_generic_basepath = variable_get('islandora_generic_base_dir', $islandora_generic_basepath));
  $db_paths = array($islandora_generic_basepath);

// convert each content object to array
  while (count($dirs_to_process) > 0) {
    $path = array_shift($dirs_to_process);
    $metadata = $dropbox->metadata($path);
    $contents = $metadata->contents;
    if ($contents) {
      foreach ($contents as $content) {
        if ($content->is_dir) {
          $dirs_to_process[] = $content->path;
        }
        $db_paths[] = $content->path;
      }
    }
  }


// get all pids, with corresponding paths to all objects decendent from
  $repository = new IslandoraTuque();
  $query = "select \$pid \$path from  <#ri>
            where \$pid <http://islandora.ca/ontology/relsext#isDecendentOf> '$root_pid'
            and   \$pid <http://islandora.ca/ontology/relsext#hasDBpath> \$path";
  $response = $repository->repository->ri->itqlQuery($query);

  $pid = $response[0]['pid']['value'];

  foreach ($response as $object) {
    $fedora_paths[] = $object['path']['value'];
    $path_mappings[$object['path']['value']] = $object['pid']['value'];
  }
  $orphans = array_diff($fedora_paths, $db_paths);

  $list = array();
  foreach ($orphans as $orphan) {
    $orphan_pid = $path_mappings[$orphan];
    $list[] = l($orphan, "islandora/object/$orphan_pid");
    $fedora_object = islandora_object_load($orphan_pid);
    $fedora_object->relationships->remove(ISLANDORA_RELS_EXT_URI, IS_DB_ORPHAN);
    $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, IS_DB_ORPHAN, 'TRUE', TRUE);
  }

  $collection_object = islandora_object_load($root_pid);
  $title = t("Orphaned objects from @label @pid", array('@label' => $collection_object->label, '@pid' => "($root_pid)"));
  $type = 'ul';
  $attributes = array('class' => 'orphaned_objects');

  $contents = theme_item_list(array('items' => $list, 'title' => $title, 'type' => 'ul', 'attributes' => $attributes));
  return $contents;
}

