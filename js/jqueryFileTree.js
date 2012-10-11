// jQuery File Tree Plugin
//
// Version 1.01
//
// Cory S.N. LaViska
// A Beautiful Site (http://abeautifulsite.net/)
// 24 March 2008
//
// Visit http://abeautifulsite.net/notebook.php?article=58 for more information
//
// Usage: $('.fileTreeDemo').fileTree( options, callback )
//
// Options:  root           - root folder to display; default = /
//           script         - location of the serverside AJAX file to use; default = jqueryFileTree.php
//           folderEvent    - event to trigger expand/collapse; default = click
//           expandSpeed    - default = 500 (ms); use -1 for no animation
//           collapseSpeed  - default = 500 (ms); use -1 for no animation
//           expandEasing   - easing function to use on expand (optional)
//           collapseEasing - easing function to use on collapse (optional)
//           multiFolder    - whether or not to limit the browser to one subfolder at a time
//           loadMessage    - Message to display while initial tree loads (can be HTML)
//
// History:
//
// 1.01 - updated to work with foreign characters in directory/file names (12 April 2008)
// 1.00 - released (24 March 2008)
//
// TERMS OF USE
//
// This plugin is dual-licensed under the GNU General Public License and the MIT License and
// is copyright 2008 A Beautiful Site, LLC.
//
//function to parse urls


(function($){

  base = window.location.toString().split('/islandora_generic')[0];

  $.ajax({
    url:base + '/islandora_generic/filetree_setup',
    async:false,
    success: function(data, status, xhr) {
      collectionRoot = data;
    },
    error: function() {
      alert("AJAX call failed");
    },
    dataType: 'html'
  });

  $.extend($.fn, {
    fileTree: function(o, h) {
     
      // Defaults
      if( !o ) var o = {};
      if( o.root == undefined ) o.root = '/';
      if(o.pid == undefined) o.pid = collectionRoot;
      if( o.script == undefined ) o.script = base + '/islandora_generic/sync';
      if( o.folderEvent == undefined ) o.folderEvent = 'click';
      if( o.expandSpeed == undefined ) o.expandSpeed= 500;
      if( o.collapseSpeed == undefined ) o.collapseSpeed= 500;
      if( o.expandEasing == undefined ) o.expandEasing = null;
      if( o.collapseEasing == undefined ) o.collapseEasing = null;
      if( o.multiFolder == undefined ) o.multiFolder = true;
      if( o.loadMessage == undefined ) o.loadMessage = 'Loading...';
      var urlparts =  window.location.toString().split('/');
      var collection = urlparts[urlparts.length -1];
      collection = collection.replace('%3A', ':');
      if(collection.indexOf(':') != -1){
        o.pid = collection;
      }
  
      $(this).each( function() {

        function showTree(c, t, p) {
          $(c).addClass('wait');
          $(".jqueryFileTree.start").remove();
          $.post(o.script, {
            dir: t,
            pid: p
          }, function(data) {
            $(c).find('.start').html('');
            $(c).removeClass('wait').append(data);
            if( o.root == t ) $(c).find('UL:hidden').show(); else $(c).find('UL:hidden').slideDown({
              duration: o.expandSpeed,
              easing: o.expandEasing
            });
            bindTree(c);
          });
        }

        function bindTree(t) {
          $(t).find('LI A').bind(o.folderEvent, function() {
            if( $(this).parent().hasClass('directory') ) {
              if( $(this).parent().hasClass('collapsed') ) {
                // Expand
                if( !o.multiFolder ) {
                  $(this).parent().parent().find('UL').slideUp({
                    duration: o.collapseSpeed,
                    easing: o.collapseEasing
                  });
                  $(this).parent().parent().find('LI.directory').removeClass('expanded').addClass('collapsed');
                }
                $(this).parent().find('UL').remove(); // cleanup
                showTree( $(this).parent(), escape($(this).attr('rel').match( /.*\// )), escape($(this).attr('pid')) );
                $(this).parent().removeClass('collapsed').addClass('expanded');
              } else {
                // Collapse
                $(this).parent().find('UL').slideUp({
                  duration: o.collapseSpeed,
                  easing: o.collapseEasing
                });
                $(this).parent().removeClass('expanded').addClass('collapsed');
              }
            } else {
              
              h($(this).attr('pid'));
            }
            return false;
          });
          // Prevent A from triggering the # on non-click events
          if( o.folderEvent.toLowerCase != 'click' ) $(t).find('LI A').bind('click', function() {
            return false;
          });
        }
        // Loading message
        $(this).html('<ul class="jqueryFileTree start"><li class="wait">' + o.loadMessage + '<li></ul>');
        // Get the initial file list
     
        showTree( $(this), escape(o.root), escape(o.pid));
      });
    }

  });



})(jQuery);



(function($){
  $(document).ready(function(){
    $('#fileview').fileTree({
      root: '/'
    }, function(pid) {
      window.open(base + '/islandora/object/' + pid);
    });

    $.contextMenu({
      selector: '.dropbox_file_item',
      callback: function(key, options) {

        var urn = $(this).parent('div').attr('urn');
       
        var title = $(this).text().substring(2,100);
        title = title.trim();

        var comment_text = $(this).next('.comment_text');
        var anno_type = comment_text.find('.comment_type').text();

        if(key == 'delete'){
          if (confirm("Permananently Delete Annotation '" + title + "'")) {
            pb_deleteAnno(urn);
          }

        }

        if(key == 'edit'){
          $(this).addClass('annotation-opened').next().show();
          var annotation = comment_text.find('.comment_content').text();
          var pm = $(this).find('.comment_showhide');
          if (pm.text() == '+ ') {
            pm.empty().append('- ');
            console.log(this);
            var id = $(this).attr('id').substring(5,100);
            var canvas = $(this).attr('canvas');
            paint_commentAnnoTargets(this, canvas, id);
          }
          startEditting(title, annotation, anno_type, urn)
        }
      },
      items: {
        "edit": {
          name: "Edit",
          icon: "edit",
          accesskey: "e"
        },
        "delete": {
          name: "Delete annotation",
          icon: "delete"
        }

      }
    });
  });
})(jQuery);
