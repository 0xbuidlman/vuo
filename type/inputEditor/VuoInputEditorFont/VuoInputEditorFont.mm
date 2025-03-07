/**
 * @file
 * VuoInputEditorFont implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorFont.hh"

// https://b33p.net/kosada/node/7282#comment-25535
#if defined(slots)
#undef slots
#endif

#define NS_RETURNS_INNER_POINTER
#include <AppKit/AppKit.h>
#include <objc/runtime.h>

/// @todo make these non-global --- how can we pass them through to changeFont and changeAttributes?
static VuoInputEditorFont *currentEditor;		///< The current font editor.
static NSString *currentFontName;				///< The current font name (PostScript format).
static double currentPointSize;					///< The current font size (in typographic points).
static bool currentUnderline;					///< Does the current font have an underline?
static NSColor *currentColor;					///< The current font's color.
static VuoHorizontalAlignment currentAlignment;	///< The current font's horizontal alignment.
static double currentCharacterSpacing;			///< The current font's character spacing.
static double currentLineSpacing;				///< The current font's line spacing.

/**
 * Constructs a VuoInputEditorFont object.
 */
VuoInputEditor * VuoInputEditorFontFactory::newInputEditor()
{
	return new VuoInputEditorFont();
}

/**
 * Returns a value indicating which font panel widgets should be displayed.
 */
static unsigned long validateFontPanelModes(id self, SEL _cmd, NSFontPanel *fontPanel)
{
   return NSFontPanelUnderlineEffectModeMask | NSFontPanelTextColorEffectModeMask | NSFontPanelCollectionModeMask | NSFontPanelFaceModeMask | NSFontPanelSizeModeMask;
}

/**
 * Returns the currently-selected font.
 */
static VuoFont getCurrentVuoFont(void)
{
	// Convert to RGB colorspace (since the color picker might return a grey or CMYK color).
	currentColor = [[currentColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace] retain];

	return VuoFont_make(
					VuoText_make([currentFontName UTF8String]),
					currentPointSize,
					currentUnderline,
					VuoColor_makeWithRGBA([currentColor redComponent], [currentColor greenComponent], [currentColor blueComponent], [currentColor alphaComponent]),
					currentAlignment,
					currentCharacterSpacing,
					currentLineSpacing
					);
}

/**
 * Returns the currently-selected attributes.
 */
static NSDictionary *getCurrentAttributes(void)
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithInteger:currentUnderline?kCTUnderlineStyleSingle:kCTUnderlineStyleNone], kCTUnderlineStyleAttributeName,
			[currentColor retain], @"NSColor",
			nil];
}

/**
 * Updates the composition's constant value when the user changes the selected font.
 */
static void userChangedFont(id self, SEL _cmd, NSFontManager *fm)
{
	NSFont *oldFont = [NSFont userFontOfSize:12];
	NSFont *newFont = [fm convertFont:oldFont];

	currentFontName = [newFont fontName];
	currentPointSize = [newFont pointSize];

	currentEditor->currentFontChanged(getCurrentVuoFont());
}

/**
 * Updates the composition's constant value when the user changes the selected attributes.
 */
static void userChangedAttributes(id self, SEL _cmd, NSFontManager *fm)
{
	NSDictionary *newAttributes = [fm convertAttributes:getCurrentAttributes()];

	NSNumber *underlineNumber = [newAttributes objectForKey:(NSString *)kCTUnderlineStyleAttributeName];
	if (underlineNumber && [underlineNumber integerValue] != kCTUnderlineStyleNone)
		currentUnderline = true;
	else
		currentUnderline = false;

	currentColor = [[newAttributes objectForKey:@"NSColor"] retain];

	currentEditor->currentFontChanged(getCurrentVuoFont());
}

/**
 * Creates a proxy QTextEdit widget.
 */
VuoInputEditorFontTextEdit::VuoInputEditorFontTextEdit(QWidget *parent)
	: QTextEdit(parent)
{
}

/**
 * When the text edit widget becomes visible, activate the font panel.
 */
void VuoInputEditorFontTextEdit::focusInEvent(QFocusEvent *event)
{
	// Show the font panel.
	NSFontManager *fm = [NSFontManager sharedFontManager];
	[fm orderFrontFontPanel:nil];

	// Give it focus (apparently orderFrontFontPanel: alone doesn't do that).
	NSFontPanel *fp = [fm fontPanel:YES];
	[fp makeKeyWindow];
}

/**
 * Intercepts events from NSFontPanel and passes them to the parent QDialog.
 */
@interface VuoInputEditorFontPanelDelegate : NSObject <NSWindowDelegate>
{
	QDialog *dialog;	///< The NSFontPanel's parent QDialog.
}
@end
@implementation VuoInputEditorFontPanelDelegate
- (id)initWithQDialog:(QDialog *)_dialog
{
	dialog = _dialog;
	return [super init];
}
- (void)windowWillClose:(NSNotification *)notification
{
	// If the user clicks the font panel's close button, also dismiss its parent modal dialog.
	dialog->accept();
}
- (void)okButtonPressed
{
	dialog->accept();
}
- (void)cancelButtonPressed
{
	dialog->reject();
}
@end

/**
 * Receives events changing the font's alignment and spacing.
 */
@interface VuoInputEditorFontAccessoryDelegate : NSObject
{
}
@end
@implementation VuoInputEditorFontAccessoryDelegate
- (void)alignmentChanged:(NSSegmentedControl *)control
{
	currentAlignment = (VuoHorizontalAlignment)[control selectedSegment];
	currentEditor->currentFontChanged(getCurrentVuoFont());
}

- (void)charSpacingChanged:(NSSlider *)control
{
	currentCharacterSpacing = [control doubleValue];
	currentEditor->currentFontChanged(getCurrentVuoFont());
}
- (void)lineSpacingChanged:(NSSlider *)control
{
	currentLineSpacing = [control doubleValue];
	currentEditor->currentFontChanged(getCurrentVuoFont());
}
@end

/**
 * Displays a Font-picker dialog.
 */
json_object * VuoInputEditorFont::show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues)
{
	// QFontDialog is unusably buggy (see https://b33p.net/kosada/node/6966#comment-24042), so we're directly invoking Cocoa's NSFontPanel.

	// NSFontPanel is a panel (a nonmodal auxiliary window), but Vuo's input editors are designed to be modal, so we have to trick it into being modal.
	// Create a modal dialog to catch events for us.
	QDialog *dialog = new QDialog;
	dialog->resize(1,1);
	dialog->setWindowFlags(Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint);

	// NSFontPanel applies its changes to the first object on the responder chain.
	// So create a text editor and inject some methods to catch NSFontPanel's changes.
	{
		VuoInputEditorFontTextEdit *textEdit = new VuoInputEditorFontTextEdit(dialog);
		NSView *textEditView = (NSView *)textEdit->winId();
		class_addMethod([textEditView class], @selector(validModesForFontPanel:), (IMP)validateFontPanelModes, "L@:@");
		class_addMethod([textEditView class], @selector(changeFont:), (IMP)userChangedFont, "v@:@");
		class_addMethod([textEditView class], @selector(changeAttributes:), (IMP)userChangedAttributes, "v@:@");
		currentEditor = this;
	}

	NSFontManager *fm = [NSFontManager sharedFontManager];
	NSFontPanel *fp = [fm fontPanel:YES];

	[[NSColorPanel sharedColorPanel] setShowsAlpha:YES];

	// QColorDialog (used in VuoInputEditorColor) reparents the sharedColorPanel's contentView when NoButtons=false;
	// when the dialog is shown by the Font editor, the buttons cause a crash.
	// By setting NoButtons=true, then discreetly opening the dialog, we force Qt to switch the sharedColorPanel back to normal.
	// https://b33p.net/kosada/node/7892
	{
		[[NSColorPanel sharedColorPanel] setAlphaValue:0];
		QColorDialog dialog;
		dialog.setOption(QColorDialog::NoButtons, true);
		dialog.show();
		dialog.hide();
		[[NSColorPanel sharedColorPanel] setAlphaValue:1];
	}

	// Create a delegate to catch if the user closes the font panel.
	VuoInputEditorFontPanelDelegate *delegate = [[VuoInputEditorFontPanelDelegate alloc] initWithQDialog:dialog];
	[fp setDelegate:delegate];

	// Preset the font panel with the port's original font.
	{
		VuoFont originalVuoFont = VuoFont_makeFromJson(originalValue);
		NSFont *originalFont;
		if (originalVuoFont.fontName)
		{
			currentFontName = [NSString stringWithUTF8String:originalVuoFont.fontName];
			currentPointSize = originalVuoFont.pointSize;
			originalFont = [NSFont fontWithName:currentFontName size:currentPointSize];
		}
		else
		{
			originalFont = [NSFont userFontOfSize:12];
			currentFontName = [originalFont fontName];
			currentPointSize = [originalFont pointSize];
		}
		[fm setSelectedFont:originalFont isMultiple:NO];

		currentColor = [[NSColor colorWithCalibratedRed:originalVuoFont.color.r green:originalVuoFont.color.g blue:originalVuoFont.color.b alpha:originalVuoFont.color.a] retain];

		currentUnderline = originalVuoFont.underline;
		[fm setSelectedAttributes:getCurrentAttributes() isMultiple:NO];

		currentAlignment = originalVuoFont.alignment;

		currentCharacterSpacing = originalVuoFont.characterSpacing;

		currentLineSpacing = originalVuoFont.lineSpacing;
	}

	// Position the font panel left of the port from which it was invoked.
	{
		NSSize panelSize = [fp frame].size;
		NSSize screenSize = [[fp screen] frame].size;
		[fp setFrameTopLeftPoint:NSMakePoint(portLeftCenter.x() - panelSize.width, screenSize.height - portLeftCenter.y() + panelSize.height/2)];
	}


	const unsigned int buttonWidth = 80;
	const unsigned int buttonHeight = 24;
	const unsigned int buttonSep = 12;
	NSView *accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 4*buttonWidth+3*buttonSep, 2*buttonHeight+3*buttonSep)];
	[fp setAccessoryView:accessoryView];
	[accessoryView autorelease];
	[fp setMinSize:NSMakeSize([accessoryView frame].size.width, [fp minSize].height)];

	VuoInputEditorFontAccessoryDelegate *accessoryDelegate = [VuoInputEditorFontAccessoryDelegate new];
	[accessoryDelegate autorelease];

	// Add a horizontal alignment widget.
	{
		NSSegmentedControl *alignmentControl = [[NSSegmentedControl alloc] initWithFrame:NSMakeRect(0, 0, buttonWidth+buttonSep+buttonWidth, buttonHeight)];
		[alignmentControl setSegmentCount:3];
		[alignmentControl setSelectedSegment:currentAlignment];

		[alignmentControl setImage:[NSImage imageNamed:@"align-left"] forSegment:0];
		[alignmentControl setImage:[NSImage imageNamed:@"align-center"] forSegment:1];
		[alignmentControl setImage:[NSImage imageNamed:@"align-right"] forSegment:2];

		[alignmentControl setTarget:accessoryDelegate];
		[alignmentControl setAction:@selector(alignmentChanged:)];

		[accessoryView addSubview:alignmentControl];
		[alignmentControl autorelease];
	}

	int secondBaseline = buttonHeight+buttonSep;
	int sliderWidth = buttonWidth+buttonSep+buttonWidth;

	// Add character spacing widget.
	{
		NSTextField *charSpacingLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(0, secondBaseline+buttonSep, sliderWidth, buttonHeight)];
		[charSpacingLabel setEditable:NO];
		[charSpacingLabel setBordered:NO];
		[charSpacingLabel setDrawsBackground:NO];
		[charSpacingLabel setStringValue:@"Character Spacing"];
		[charSpacingLabel setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
		[accessoryView addSubview:charSpacingLabel];
		[charSpacingLabel autorelease];

		NSSlider *charSpacingControl = [[NSSlider alloc] initWithFrame:NSMakeRect(0, secondBaseline, sliderWidth, buttonHeight)];
		[charSpacingControl setNumberOfTickMarks:1];	// A single tick at the center.
		[charSpacingControl setMinValue:0];
		[charSpacingControl setMaxValue:2];
		[charSpacingControl setDoubleValue:currentCharacterSpacing];
		[[charSpacingControl cell] setControlSize:NSSmallControlSize];

		[charSpacingControl setTarget:accessoryDelegate];
		[charSpacingControl setAction:@selector(charSpacingChanged:)];

		[accessoryView addSubview:charSpacingControl];
		[charSpacingControl autorelease];
	}

	// Add line spacing widget.
	{
		NSTextField *lineSpacingLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(2*buttonWidth+2*buttonSep, secondBaseline+buttonSep, sliderWidth, buttonHeight)];
		[lineSpacingLabel setEditable:NO];
		[lineSpacingLabel setBordered:NO];
		[lineSpacingLabel setDrawsBackground:NO];
		[lineSpacingLabel setStringValue:@"Line Spacing"];
		[lineSpacingLabel setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
		[accessoryView addSubview:lineSpacingLabel];
		[lineSpacingLabel autorelease];

		NSSlider *lineSpacingControl = [[NSSlider alloc] initWithFrame:NSMakeRect(2*buttonWidth+2*buttonSep, secondBaseline, sliderWidth, buttonHeight)];
		[lineSpacingControl setNumberOfTickMarks:1];	// A single tick at the center.
		[lineSpacingControl setMinValue:0];
		[lineSpacingControl setMaxValue:2];
		[lineSpacingControl setDoubleValue:currentLineSpacing];
		[[lineSpacingControl cell] setControlSize:NSSmallControlSize];

		[lineSpacingControl setTarget:accessoryDelegate];
		[lineSpacingControl setAction:@selector(lineSpacingChanged:)];

		[accessoryView addSubview:lineSpacingControl];
		[lineSpacingControl autorelease];
	}


	// Provide another way to dismiss the font panel (more visible than just the panel's close button).
	{
		{
			NSButton *okButton = [[NSButton alloc] initWithFrame:NSMakeRect(3*buttonWidth+3*buttonSep,0,buttonWidth,buttonHeight)];
			[okButton setKeyEquivalent:@"\r"];	// Return
			[okButton setTitle:@"OK"];
			[okButton setButtonType:NSMomentaryLightButton];
			[okButton setBezelStyle:NSRoundedBezelStyle];
			[okButton setTarget:delegate];
			[okButton setAction:@selector(okButtonPressed)];
			[accessoryView addSubview:okButton];
			[okButton autorelease];
		}
		{
			NSButton *cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(2*buttonWidth+3*buttonSep,0,buttonWidth,buttonHeight)];
			[cancelButton setKeyEquivalent:@"\E"];	// Escape
			[cancelButton setTitle:@"Cancel"];
			[cancelButton setButtonType:NSMomentaryLightButton];
			[cancelButton setBezelStyle:NSRoundedBezelStyle];
			[cancelButton setTarget:delegate];
			[cancelButton setAction:@selector(cancelButtonPressed)];
			[accessoryView addSubview:cancelButton];
			[cancelButton autorelease];
		}
	}

	// Wait for the user to dismiss the font panel.
	int result = dialog->exec();


	// Hide the color picker.
	[[NSColorPanel sharedColorPanel] orderOut:nil];

	// Hide the font panel.
	[fp orderOut:nil];

	[delegate release];

	currentEditor = NULL;

	if (result == QDialog::Accepted)
		return VuoFont_getJson(getCurrentVuoFont());
	else
		return originalValue;
}

/**
 * Fired when the user changes the font in the font dialog.
 */
void VuoInputEditorFont::currentFontChanged(VuoFont font)
{
	json_object *valueAsJson = VuoFont_getJson(font);
	emit valueChanged(valueAsJson);
	json_object_put(valueAsJson);
}
