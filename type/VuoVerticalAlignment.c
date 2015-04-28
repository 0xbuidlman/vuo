/**
 * @file
 * VuoVerticalAlignment implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoVerticalAlignment.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Vertical Alignment",
					  "description" : "Vertical alignment.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "c"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoVerticalAlignment
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "replaceThis" : -1
 *   }
 * }
 */
VuoVerticalAlignment VuoVerticalAlignment_valueFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "center") == 0)
		return VuoVerticalAlignment_Center;
	else if (strcmp(valueAsString, "bottom") == 0)
		return VuoVerticalAlignment_Bottom;

	return VuoVerticalAlignment_Top;
}

/**
 * @ingroup VuoVerticalAlignment
 * Encodes @c value as a JSON object.
 */
json_object * VuoVerticalAlignment_jsonFromValue(const VuoVerticalAlignment value)
{
	char *valueAsString = "top";

	if (value == VuoVerticalAlignment_Center)
		valueAsString = "center";
	else if (value == VuoVerticalAlignment_Bottom)
		valueAsString = "bottom";

	return json_object_new_string(valueAsString);
}

/**
 * @ingroup VuoVerticalAlignment
 * Returns a compact string representation of @c value.
 */
char * VuoVerticalAlignment_summaryFromValue(const VuoVerticalAlignment value)
{
	char *valueAsString = "Top";

	if (value == VuoVerticalAlignment_Center)
		valueAsString = "Center";
	else if (value == VuoVerticalAlignment_Bottom)
		valueAsString = "Bottom";

	return strdup(valueAsString);
}
