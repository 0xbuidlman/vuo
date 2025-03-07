/**
 * @file
 * VuoRssItem implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoRssItem.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "RSS Feed Item",
					  "description" : "An item from an RSS feed.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoImage",
						  "VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * An example from https://vuo.org/composition/feed :
 * @eg{
 *   {
 *     "title": "Parallax Scrolling",
 *     "author": "smokris",
 *     "description": "<p>A simple example of a few concepts:</p>…",  // UTF8 HTML text
 *     "url": "https://vuo.org/node/533",
 *     "image": {"glTextureName": 42, …}
 *   }
 * }
 */
VuoRssItem VuoRssItem_makeFromJson(json_object * js)
{
	VuoRssItem value = {NULL, NULL, NULL, NULL, NULL};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "title", &o))
		value.title = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "author", &o))
		value.author = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "description", &o))
		value.description = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "url", &o))
		value.url = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "image", &o))
		value.image = VuoImage_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoRssItem_getJson(const VuoRssItem value)
{
	json_object *js = json_object_new_object();

	json_object *titleObject = VuoText_getJson(value.title);
	json_object_object_add(js, "title", titleObject);

	json_object *authorObject = VuoText_getJson(value.author);
	json_object_object_add(js, "author", authorObject);

	json_object *descriptionObject = VuoText_getJson(value.description);
	json_object_object_add(js, "description", descriptionObject);

	json_object *urlObject = VuoText_getJson(value.url);
	json_object_object_add(js, "url", urlObject);

	json_object *imageObject = VuoImage_getJson(value.image);
	json_object_object_add(js, "image", imageObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoRssItem_getSummary(const VuoRssItem value)
{
	return VuoText_getSummary(value.title);
}
