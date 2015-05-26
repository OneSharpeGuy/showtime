(function ($) {
  Drupal.behaviors.atheadliner_egp = {
    attach: function(context) {
      $('#tri-panel .block-content').equalHeight();
      $('#bi-panel .block-content').equalHeight();
      $('#quad-panel .block-content').equalHeight();
    }
  };
})(jQuery);