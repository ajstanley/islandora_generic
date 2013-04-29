<?php
/**
 * @file
 * Callbacks for dropbox synchronization.
 */

/**
 * Assembles information for treeview
 *   Called by AJAX.  Paramters passed through $_POST.
 * @return json
 *   HTML for the js treeview
 */
function islandora_generic_filetree_sync() {
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora', 'includes/mime.detect');
  $mime_helper = new MimeDetect();
  $_POST['dir'] = urldecode($_POST['dir']);
  $collection_pid = check_plain(urldecode($_POST['pid']));
  $path = check_plain(urldecode($_POST['dir']));
  if ($path != '/') {
    $path = rtrim($path, '/');
  }
  if (!$collection_pid) {
    return;
  }
  $collection_object = islandora_object_load($collection_pid);
  if (!isset($collection_object)) {
    echo(t("Dropbox Sync does not appear to be correctly configured."));
    return;
  }
  $query = islandora_generic_get_collection_query($collection_pid);
  $objects = $collection_object->repository->ri->sparqlQuery($query, 'unlimited');
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
      $mimetypes = $collection_object->repository->ri->sparqlQuery($mime_query, 'unlimited');
      $mime_type = $mimetypes[0]['object']['value'];
      $ext = $mime_helper->getExtension($mime_type);

      $filestring .= "<li title = '$mime_type' class='file ext_$ext dropbox_file_item'><a href='#' class = 'dropbox_file_item' rel='$rel'  pid='$filepid'>$label</a></li>";
    }
  }
  echo "<ul class='jqueryFileTree' style='display: none;'>$dirstring $filestring</ul>";
}

/**
 * Adds a single div to the Drupal page to be populated by javascript
 *
 * @param type $fileview_basedir
 *   Optional base directory for treeview.
 *
 * @return string
 *   The HTML for page.
 */
function islandora_generic_filetree($fileview_basedir = NULL) {
  $path = drupal_get_path('module', 'islandora_generic');
  if (!$fileview_basedir) {
    $fileview_basedir = variable_get('islandora_generic_collection_pid', 'islandora:root');
  }
  drupal_add_js("$path/js/jqueryFileTree.js");
  drupal_add_js(array('islandora_fileview' => array('fileview_basedir' => $fileview_basedir)), 'setting');
  drupal_add_css("$path/css/islandora_generic_treeview.css");

  $output = "<div id='fileview'>Content will go here</div>";
  return $output;
}

/**
 * Retrieves all child objects of a given collection
 *
 * @param type $collection_pid
 *   PID of collection being queried.
 *
 * @return string
 *   Sparql query.
 */
function islandora_generic_get_collection_query($collection_pid) {
  $query = <<<EOQ
            PREFIX fm: <info:fedora/fedora-system:def/model#>
            PREFIX fr: <info:fedora/fedora-system:def/relations-external#>
            SELECT ?object ?model ?label
            FROM <#ri>
            WHERE {
              {?object fm:hasModel ?model;
                       fr:isMemberOfCollection <info:fedora/$collection_pid>;
              }
            UNION
                    {?object fm:hasModel ?model;
                       fr:isMemberOf <info:fedora/$collection_pid>;
              }
            ?object fm:state fm:Active;
            FILTER (?model != <info:fedora/fedora-system:FedoraObject-3.0>)
            OPTIONAL{
                 ?object fm:label ?label
            }
           }
EOQ;
  return $query;
}

/**
 * Returns object's mimetype.
 *
 * @param string $pid
 *   PID of object being examined
 *
 * @return string
 *   Sparql query.
 */
function islandora_generic_get_mimetype_query($pid) {
  $query = <<<EOQ
            PREFIX fm: <info:fedora/fedora-system:def/view#>
            SELECT ?object
            FROM <#ri>
            WHERE {
               <info:fedora/$pid/OBJ> fm:mimeType ?object
            }
EOQ;
  return $query;
}
