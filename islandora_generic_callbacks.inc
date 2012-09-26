<?php

function islandora_generic_filetree_setup() {
  module_load_include('inc', 'islandora_generic', 'islandora_generic_database');
  $_POST['dir'] = urldecode($_POST['dir']);
  $path = urldecode($_POST['dir']);
  if ($path != '/') {
    $path = rtrim($path, '/');
  }
  $collection_pid = islandora_generic_get_pid_from_path($path);
  if (!$collection_pid) {
    return;
  }
  $collection_object = islandora_object_load($collection_pid);
  $query = islandora_generic_get_collection_query($collection_pid);
  $objects = $collection_object->repository->ri->itqlQuery($query, 'unlimited', '0');
  $dirstring = '';
  $filestring = '';

  foreach ($objects as $object) {
    if ($object['model']['value'] == 'islandora:collectionCModel') {
      $dirstring .= "<li class=\"directory collapsed\"><a href=\"#\" rel=\"" . htmlentities($path . $object['label']['value']) . "/\">" . $object['label']['value'] . "</a></li>";
    }
    else {
      $filestring .= "<li class=\"file ext_pdf\"><a href=\"#\" rel=\"" . htmlentities($path . $object['label']['value']) . "\">" . $object['label']['value'] . "</a></li>";
    }
  }
  echo "<ul class=\"jqueryFileTree\" style=\"display: none;\">" .$dirstring . $filestring ."</ul>";

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

function islandora_generic_get_mimetype($pid){
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