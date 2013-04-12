<?php
/**
 * @file
 * events_cdb.sync.inc
 */

/**
 * Implements hook_node_presave().
 */
function events_cdb_node_presave($node) {
  
  if ($node->type == 'event_cdb') {
    
    $update = FALSE;
    if (isset($node->{'field_cdbid'}['und'][0]['value'])) {
      $update = TRUE;
      $event = DrupalCultureFeed_EntryApi::getEvent($node->{'field_cdbid'}['und'][0]['value']);
    }
    else {
      $event = new CultureFeed_Cdb_Item_Event();
    }
    
    // Timestamps as calendar
    $calendar = new CultureFeed_Cdb_Data_Calendar_TimestampList();
    foreach ($node->{'field_date'}['und'] as $timestamp) {
      
      $timestamp_object = new CultureFeed_Cdb_Data_Calendar_Timestamp(date("Y-m-d", strtotime($timestamp['value'])));
      $timestamp_object->setStartTime(date("H:i:s", strtotime($timestamp['value'])));

      $calendar->add($timestamp_object);
    }
    $event->setCalendar($calendar);
    

    // Categories
    $categories = new CultureFeed_Cdb_Data_CategoryList();
    $terms = taxonomy_get_tree(2, 0, NULL, TRUE);

    $term_cdbids = array();
    $term_names = array();
    foreach ($terms as $term) {
      $term_cdbids[$term->{'tid'}] = $term->{'field_cdbid'}['und'][0]['value'];
      $term_names[$term->{'tid'}] = $term->{'name'};
    }
    foreach ($node->{'field_category'}['und'] as $category) {
      $categories->add(new CultureFeed_Cdb_Data_Category(CultureFeed_Cdb_Data_Category::CATEGORY_TYPE_EVENT_TYPE, $term_cdbids[$category['tid']], $term_names[$category['tid']] ));
    }
    $event->setCategories($categories);
  
  
    // Event details
    $detail = new CultureFeed_Cdb_Data_EventDetail();
    
    $detail->setTitle($node->title);
    
    if (isset($node->{'field_short_description'})) {
      $detail->setShortDescription(strip_tags(substr($node->{'field_short_description'}['und'][0]['value'],0,400)));
    }
    
    if (isset($node->body) && !empty($node->body)) {
      $detail->setLongDescription($node->body['und'][0]['value']);
    }
    
    $detail->setLanguage('nl');

    
    if (isset($node->{'field_price'}['und'][0])) {
      $price = new CultureFeed_Cdb_Data_Price($node->{'field_price'}['und'][0]['value']);
      $description = '';
      if (isset($node->{'field_presale'})) {
        $description = 'In voorverkoop: ' . $node->{'field_presale'}['und'][0]['value'] . ' ';
      }
      if (isset($node->{'field_price_information'})) {
        $description .= $node->{'field_price_information'}['und'][0]['value'];
      }
      $price->setDescription($description);
      $detail->setPrice($price);
    }

    // Media - links
    if (isset($node->{'field_links'})) {

      foreach ($node->{'field_links'}['und'] as $media) {
        
        $file = new CultureFeed_Cdb_Data_File();
        $file->setHLink($media['url']);
        $file->setMediaType(CultureFeed_Cdb_Data_File::MEDIA_TYPE_WEBRESOURCE);
        if (isset($media['title'])) {
          $file->setTitle($media['title']);
        }
        $detail->getMedia()->add($file);
      }
    }
    
    // Media -  image
    if (isset($node->{'original'}->{'field_photo'}['und'][0]['uri'])) {
      
      $file = new CultureFeed_Cdb_Data_File();
      $file->setHLink(str_replace('local','www',image_style_url('large', $node->{'original'}->{'field_photo'}['und'][0]['uri'])));
      $file->setMediaType(CultureFeed_Cdb_Data_File::MEDIA_TYPE_PHOTO);
      $file->setFileName($node->{'original'}->{'field_photo'}['und'][0]['filename']);
      $filetype = str_replace('image/', '', $node->{'original'}->{'field_photo'}['und'][0]['filemime']);
      $file->setFileType($filetype);
      $file->setMain(TRUE);
      if (!empty($node->{'original'}->{'field_photo'}['und'][0]['title'])) {
        $file->setCopyright($node->{'original'}->{'field_photo'}['und'][0]['title']);
      }
      else {
        $file->setCopyright($node->title);
      }
      
      $detail->getMedia()->add($file);    
    }
    
    $details = new CultureFeed_Cdb_Data_EventDetailList();
    $details->add($detail);
    $event->setDetails($details);


    // Location / address
    $location_label = 'Den Hemel';
    $location_cdbid = '31410A4E-9851-1D6A-788BCF043C72A4D2';

    // Because we don't have a getActor in the php lib we do it the old way, to be fixed in next release
    $xml = simplexml_load_file('http://build.uitdatabank.be/api/actor/' . $location_cdbid . '?key=1');
    $actor = $xml->actors->actor;
    $location = $actor->contactinfo->address->physical;
    
    $physical_address = new CultureFeed_Cdb_Data_Address_PhysicalAddress();
    $physical_address->setCity($location->city);
    $physical_address->setCountry($location->country);
    $physical_address->setZip($location->zipcode);
    if (isset($location->street)) {
      $physical_address->setStreet($location->street);
      $physical_address->setHouseNumber($location->housenr);
    }
    $address = new CultureFeed_Cdb_Data_Address($physical_address);
  
    $cdbLocation = new CultureFeed_Cdb_Data_Location($address);
    $cdbLocation->setLabel($location_label);
    $cdbLocation->setCdbid($location_cdbid);
    $event->setLocation($cdbLocation);
  
  
    // Contact info
    $contact_object = new CultureFeed_Cdb_Data_ContactInfo();
    $contact_object->addAddress(new CultureFeed_Cdb_Data_Address($physical_address));
    if ( isset($actor->contactinfo->mail)) {
      $contact_object->addMail(new CultureFeed_Cdb_Data_Mail($actor->contactinfo->mail, FALSE, FALSE));
    }
    if ( isset($actor->contactinfo->website)) {
      $contact_object->addUrl(new CultureFeed_Cdb_Data_Url($actor->contactinfo->website, FALSE, FALSE));
    }
    if ( isset($actor->contactinfo->phone)) {
      $contact_object->addPhone(new CultureFeed_Cdb_Data_Phone($actor->contactinfo->phone, CultureFeed_Cdb_Data_Phone::PHONE_TYPE_PHONE, FALSE, FALSE));
    }
    $event->setContactInfo($contact_object);
    
  
    // Publication date
    $event->setPublicationDate(date('Y-m-d'));
    
    
    // Keywords
    if (isset($node->{'field_tags'}['und'])) {
      foreach ($node->{'field_tags'}['und'] as $tag) {
        $event->addKeyword($tag['name']);
      }
    }
    
    try {
      if ($update) {
        DrupalCultureFeed_EntryApi::updateEvent($event);
        $id = $event->getCdbId();
      }
      else {
        $node->{'field_cdbid'}['und'][0]['value'] = DrupalCultureFeed_EntryApi::createEvent($event);
      }

    }
    catch (Exception $e) {
      watchdog_exception(WATCHDOG_ERROR, $e);
      drupal_set_message('Entry API error: <pre>' . $e . '</pre>' , 'error');
    }
  
  }
}
