<?php

function islandora_generic_filetree_sync() {
  module_load_include('inc', 'islandora_generic', 'islandora_generic_database');
  module_load_include('inc', 'islandora', 'includes/mime.detect');
  $mime_helper = new MimeDetect();
  $_POST['dir'] = urldecode($_POST['dir']);
  $collection_pid = urldecode($_POST['pid']);
  $path = urldecode($_POST['dir']);
  if ($path != '/') {
    $path = rtrim($path, '/');
  }

  if (!$collection_pid) {
    return;
  }
  $collection_object = islandora_object_load($collection_pid);
  $query = islandora_generic_get_collection_query($collection_pid);
  $objects = $collection_object->repository->ri->itqlQuery($query, 'unlimited', '0');
  $dirstring = '';
  $filestring = '';

  foreach ($objects as $object) {
    $rel = htmlentities($path . $object['label']['value']);
    $label = $object['label']['value'];
    $filepid = $object['object']['value'];
    if ($object['model']['value'] == 'islandora:collectionCModel') {
      $dirstring .= "<li class='directory collapsed'><a href='#' rel='$rel' pid = '$filepid'>$label</a></li>";
    }
    else {
      $mime_query = islandora_generic_get_mimetype_query($object['object']['value']);
      $mimetypes = $collection_object->repository->ri->itqlQuery($mime_query, 'unlimited', '0');
      $mime_type = $mimetypes[0]['object']['value'];
      $ext = $mime_helper->getExtension($mime_type);

      $filestring .= "<li class='file ext_$ext dropbox_file_item'><a href='#' class = 'dropbox_file_item' rel='$rel'  pid='$filepid'>$label</a></li>";
    }
  }
  echo "<ul class='jqueryFileTree' style='display: none;'>$dirstring $filestring</ul>";
}

function islandora_generic_filetree() {
  $path = drupal_get_path('module', 'islandora_generic');
  drupal_add_js("$path/js/jqueryFileTree.js");
  drupal_add_css("$path/css/islandora_generic_treeview.css");

  $output = "<div id='fileview'>Content will go here</div>";
  return $output;
}

function islandora_generic_get_collection_query($pid) {
  $query = <<<EOQ
   select \$object  \$model \$label from <#ri>
    where ((\$object <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/$pid>
           or \$object <info:fedora/fedora-system:def/relations-external#isMemberOf> <info:fedora/$pid>)
    and \$object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>
    and \$object <info:fedora/fedora-system:def/model#hasModel> \$model
    and \$object <info:fedora/fedora-system:def/model#label> \$label)
    minus \$model <mulgara:is> <info:fedora/fedora-system:FedoraObject-3.0>
EOQ;
  return $query;
}

function islandora_generic_get_mimetype_query($pid) {
  $query = <<<EOQ
    select \$object from <#ri>
    where <info:fedora/$pid/SOURCE> <info:fedora/fedora-system:def/view#mimeType> \$object
EOQ;
  return $query;
}

function islandora_generic_filetree_setup() {
  echo variable_get('islandora_generic_collection_pid', 'islandora:root');
}