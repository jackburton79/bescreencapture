#include "messages.h"
#include "PostProcessingView.h"
#include "Settings.h"

#include <Box.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <Window.h>

#include <cstdio>


PostProcessingView::PostProcessingView(const char *name, uint32 flags)
	:
	BView(name, flags)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	
	BBox *box = new BBox("container");
	AddChild(box);
	box->SetLabel("Clip size");
	
	BRadioButton *normalSizeRB 
		= new BRadioButton("100 Original", "100\% (Original size)",
			new BMessage(kClipSizeChanged));
			
	BView *layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 15)
		.SetInsets(10, 10, 10, 10)
			.Add(new BRadioButton("200", "200\%", new BMessage(kClipSizeChanged)))
			.Add(normalSizeRB)
			.Add(new BRadioButton("50", "50\%", new BMessage(kClipSizeChanged)))
			.Add(new BRadioButton("25", "25\%", new BMessage(kClipSizeChanged)))
		.End()
		.View();
	
	box->AddChild(layoutView);

	normalSizeRB->SetValue(B_CONTROL_ON);
}


void
PostProcessingView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(Parent()->ViewColor());

	BView *container = FindView("container");
	if (container != NULL) {
		BControl *child = dynamic_cast<BControl *>(container->ChildAt(0));
		while (child != NULL) {
			child->SetTarget(this);
			child = dynamic_cast<BControl *>(child->NextSibling());
		}
	}
}


void
PostProcessingView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kClipSizeChanged:
		{
			BRadioButton *source = NULL;
			message->FindPointer("source", (void **)&source);
			if (source != NULL) {
				float num;
				sscanf(source->Name(), "%1f", &num);
				Settings().SetClipSize(num);
				Window()->PostMessage(kClipSizeChanged);
			}
			break;
		}
			
		default:
			BView::MessageReceived(message);
			break;
	}
}


/* virtual */
BSize
PostProcessingView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(400, 400));
}
