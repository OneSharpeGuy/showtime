<?php

// $Id$
/**
 * Generate markup for marinelli's banners.
 *
 * @return <string>
 *    HTML markup to printo into page
 */

function marinelli_banners_markup($banners)
{
    if ($banners && theme_get_setting('banner_usage') != 0)
    {
        // Add javascript to manage banners
        marinelli_banners_add_js(count($banners));
        // Generate HTML markup for banners
        return marinelli_banner_markup($banners);
    } else
    {
        return '';
    }
}
/**
 * Add JS required to manage banner into this page.
 *
 * @param <int> $num_banners
 *    Number of banner visible into this page
 */

function marinelli_banners_add_js($num_banners)
{
    // in this two cases we do not need cycle, we use a small script to fill banner text
    if ($num_banners <= 1)
    {
        // banners match only url
        drupal_add_js(path_to_theme() . '/js/banner/marinelli_banner_text.js');
    }
    // adds cycle passing some parameters
    else
    {
        // add the required javascript
        drupal_add_js(path_to_theme() . '/js/cycle/cycle.js');
        drupal_add_js(path_to_theme() .
            '/js/banner/marinelli_configure_cycle.js');
        // pass the text variables to javascript
        drupal_add_js(array('marinelli' => array(
                'banner_effect' => theme_get_setting('banner_effect'),
                'banner_speed' => theme_get_setting('banner_speed'),
                'banner_delay' => theme_get_setting('banner_delay'),
                'banner_pause' => theme_get_setting('banner_pause') == 1 ? 1 : 0,
                )), array('type' => 'setting'));
    }
}
/**
 * Generate banners markup.
 *
 * @return <string>
 *    HTML code to display banner markup.
 */

function marinelli_banner_markup($banners)
{
    $output = '';
    foreach ($banners as $i => $banner)
    {
        $variables = array(
            'path' => $banner['image_path'],
            'alt' => t('@image_desc', array('@image_desc' => $banner['image_description'])),
            'title' => t('@image_title', array('@image_title' => $banner['image_title'])),
            'attributes' => array(
                'class' => 'slide' . ($i != 0 ? ' marinelli-hide-no-js' : ''), // hide all the slides except #1
                'id' => 'slide-number-' . $i,
                'longdesc' => t('@image_desc', array('@image_desc' => $banner['image_description']))),
            );
        // Draw image
        $image = theme('image', $variables);
        // Remove link if is the same page
        $banner['image_url'] = ($banner['image_url'] == current_path()) ? FALSE :
            $banner['image_url'];
        // Add link (if required)
        $output .= $banner['image_url'] ? l($image, $banner['image_url'], array
            ('html' => TRUE)) : $image;
    }
    return $output;
}
/**
 * Get banner to show into current page in accord with settings
 *
 * @return <array>
 *    Banners to show
 */

function marinelli_show_banners()
{
    $banners = marinelli_get_banners(FALSE);
    $display_banners = array();
    // Current path alias
    $path = drupal_strtolower(drupal_get_path_alias($_GET['q']));
    // If Visibility Path Starts with '>' and ends with '<' only pages specified for this path will show on this page
    //Chucks's Mod vvv
    $exclusive_path = "";
    foreach ($banners as $banner_cws)
    {
        $pages = trim($banner_cws['image_visibility']);
        $flag = substr($pages, 0, 1);
        if (strlen($pages) > 1)
        {
            $flag = $flag . substr($pages, -1, 1);
        }
        if ($flag == "><")
        {
            $target = trim(substr($pages, 1, strlen($pages) - 2));
            if ($target == $path)
            {
                $exclusive_path = $pages;
                break;
            }
        }
    }
    //	echo "<!--\n";
    //	echo "exclusive_path: [$exclusive_path]\n";
    //	echo "path: [$path]\n";
    //	echo "-->\n\n";
    //Chuck's mod ^^^
    // Check visibility for each banner
    foreach ($banners as $banner)
    {
        // Pages
        $pages = drupal_strtolower($banner['image_visibility']);
        if ($pages == "*")
        {
            if ($exclusive_path > "")
            {
                $pages = "";
            }
        }
        // Check path for alias, and (if required) for path
        $page_match = drupal_match_path($path, $pages);
        if ($path != $_GET['q'])
        {
            $page_match = $page_match || drupal_match_path($_GET['q'], $pages);
        }
        //Chuck's mod vvv
        if (!$page_match)
        {
            $flag = substr($pages, 0, 1);
            if (strlen($pages) > 1)
            {
                $flag = $flag . substr($pages, -1, 1);
            }
            if ($flag == "><")
            {
                $pages = trim(substr($pages, 1, strlen($pages) - 2));
            }
            if ($pages == $path)
            {
                $page_match = TRUE;
            }
        }
        //		echo "<!--\n";
        //		$hold_cws=$banner['image_path'];
        //		echo "image: [$hold_cws]\n";
        //		echo "pages:  [$pages]\n";
        //		echo "target: [$target]\n";
        //		echo "-->\n";
        //Chuck's mod ^^^
        // Add banner to visible banner
        if ($page_match)
        {
            $display_banners[] = $banner;
        }
    }
    //Just in case banner is very large, take only the first 30 photos, but first shuffle the array
    shuffle($display_banners);

    if (count($display_banners) > 30)
    {
        $display_banners = array_slice($display_banners, 0, 30);
    }
    $display_banners = array_values($display_banners);

    return $display_banners;
}
