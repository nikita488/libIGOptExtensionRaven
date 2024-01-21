#include "../include/ExtractMotionRaven.h"

namespace Gap
{

namespace OptExtension
{

igBool ExtractMotionRaven::configure(igInt sectionHandle)
{
	return true;
}

igBool ExtractMotionRaven::apply(igNodeRef& node)
{
	return true;
}

void ExtractMotionRaven::ExtractMotionFromAnimation(igAnimationDatabase* animDB, igAnimation* animation)
{
	igAnimationTrack* rootTrack = animation->getAnimationTrack("Bip01");

	if (!rootTrack)
	{
		printf("No animation track found in animation '%s' bound to root bone 'Bip01'\n", animation->getName());
		return;
	}

	igAnimationTrack* motionTrack = animation->getAnimationTrack("Motion");

	if (!motionTrack)
	{
		printf("No animation track found in animation '%s' bound to motion bone 'Motion'\n", animation->getName());
		return;
	}

	igTransformSequence1_5* rootSequence = igTransformSequence1_5::dynamicCast(rootTrack->getSource());

	if (!rootSequence)
	{
		printf("No supported transformation source was found bound to animation track 'Bip01'. Requires 'igTransformSequence1_5'\n");
		return;
	}

	igTransformSequence1_5* motionSequence = igTransformSequence1_5::dynamicCast(motionTrack->getSource());

	if (!motionSequence)
	{
		printf("No supported transformation source was found bound to animation track 'Motion'. Requires 'igTransformSequence1_5'\n");
		return;
	}

	if ((motionSequence->getComponentChannels() & IG_MATH_TRANSFORM_TRANSLATION) == 0)
	{
		printf("The igTransformSequence1_5 'Motion' does not have the required Translation component\n");
		return;
	}

	const igInt keyCount = motionSequence->getKeyFrameCount();
	const igInt motionXlateCount = motionSequence->getTranslationCount();
	const igInt rootXlateCount = (rootSequence->getComponentChannels() & IG_MATH_TRANSFORM_TRANSLATION) ? rootSequence->getTranslationCount() : 0;
	
	if (keyCount < 2 || motionXlateCount < 2)
	{
		printf("Not enough translation keyframes were found in igTransformSequence1_5 'Motion'. At least 2 keyframes are required\n");
		return;
	}

	const igVec3f firstXlate = *motionSequence->getTranslation(0);
	const igVec3f lastXlate = *motionSequence->getTranslation(motionXlateCount - 1);
	igVec3f motionDir = igVec3f(lastXlate[0] - firstXlate[0], lastXlate[1] - firstXlate[1], lastXlate[2] - firstXlate[2]);
	igVec3f motionDirScaled;
	
	const igDouble durationNano = igDouble(rootSequence->getDuration());
	const igDouble durationSeconds = durationNano * (1 / 1000000000.0);
	
	const igBool linear = keyCount <= 3;//key count < 3 was in original Raven plugin, but animation with 3 keyframes also have w +999.0
	const igString motionType = linear ? "Linear" : "Tracked";
	
	if (durationSeconds > 0.0)
	{
		const igDouble inverseScalar = 1.0 / durationSeconds;
		motionDirScaled.set(motionDir[0] * inverseScalar, motionDir[1] * inverseScalar, motionDir[2] * inverseScalar);
	}

	if (linear)
	{
		if (abs(motionDir[0]) <= 6.0F)
			motionDir[0] = 0.0F;
		if (abs(motionDir[1]) <= 6.0F)
			motionDir[1] = 0.0F;
		if (abs(motionDir[2]) <= 6.0F)
			motionDir[2] = 0.0F;
	}

	printf("(Motion) %s - keys=%d dur=%3.3f dist=(%3.1f,%3.1f,%3.1f)\n", motionType, keyCount, durationSeconds, motionDir[0], motionDir[1], motionDir[2]);

	if (linear)
	{
		/*if (durationSeconds > 0.0 && (motionDir[0] != 0.0F || motionDir[1] != 0.0F || motionDir[2] != 0.0F))
		{
			const igDouble inverseScalar = 1.0 / durationNano;
			const igVec3f motionDirScaledNano = igVec3f(motionDir[0] * inverseScalar, motionDir[1] * inverseScalar, motionDir[2] * inverseScalar);

			for (igInt keyIndex = 0; keyIndex < rootXlateCount; keyIndex++)
			{
				const igDouble keyTime = igDouble(rootSequence->getTime(keyIndex));
				const igVec3f rootXlate = *rootSequence->getTranslation(keyIndex);
				const igVec3f dir = igVec3f(
					rootXlate[0] - (firstXlate[0] + (motionDirScaledNano[0] * keyTime)), 
					rootXlate[1] - (firstXlate[1] + (motionDirScaledNano[1] * keyTime)), 
					rootXlate[2] - (firstXlate[2] + (motionDirScaledNano[2] * keyTime)));
				
				rootSequence->setTranslation(keyIndex, &dir);
			}
		}*/
	}
	else
	{
		/*for (igInt keyIndex = 0; keyIndex < rootXlateCount; keyIndex++)
		{
			const igVec3f rootXlate = *rootSequence->getTranslation(keyIndex);
			igMatrix44f motionTM = igMatrix44f();
			igVec3f motionXlate;
			
			motionSequence->getMatrix(motionTM, rootSequence->getTime(keyIndex));
			motionTM.getTranslation(motionXlate);

			igVec3f dir = igVec3f(motionXlate[0] + rootXlate[0], motionXlate[1] + rootXlate[1], motionXlate[2] + rootXlate[2]);//rootXlate[0] - motionXlate[0], rootXlate[1] - motionXlate[1], rootXlate[2] - motionXlate[2]
			rootSequence->setTranslation(keyIndex, &dir);
		}*/

		for (igInt keyIndex = 0; keyIndex < motionXlateCount; keyIndex++)
		{
			const igVec3f motionXlate = *motionSequence->getTranslation(keyIndex);
			const igVec3f dir = igVec3f(motionXlate[0] - firstXlate[0], motionXlate[1] - firstXlate[1], motionXlate[2] - firstXlate[2]);

			motionSequence->setTranslation(keyIndex, &dir);
		}
	}

	igVec4f& constantRotation = *motionTrack->getConstantQuaternion();

	constantRotation.set(motionDirScaled);
	constantRotation[3] = linear ? 999.0F : -999.0F;
	motionTrack->getConstantTranslation()->set(firstXlate);
}

igBool ExtractMotionRaven::ApplyToDB(igAnimationDatabase* animDB)
{
	if (!animDB->getSkeletonList()->isEmpty())
	{
		igSkeleton* skeleton = animDB->getSkeleton(0);

		if (!skeleton->getBoneInfoList()->isEmpty())
		{
			if (skeleton->findBoneIndex("Bip01") < 0)
			{
				printf("The bone 'Bip01' was not found\n");
				return false;
			}
			else if (skeleton->findBoneIndex("Motion") < 0)
			{
				printf("The bone 'Motion' was not found\n");
				return false;
			}

			if (!animDB->getAnimationList()->isEmpty())
			{
				for (igInt animationIndex = 0; animationIndex < animDB->getAnimationList()->getCount(); animationIndex++)
				{
					igAnimation* animation = animDB->getAnimation(animationIndex);

					printf("Extracting Motion from Animation '%s'\n", animation->getName());
					ExtractMotionFromAnimation(animDB, animation);
				}
			}
			else
			{
				printf("No animations were found to extract motion from them\n");
				return false;
			}
		}
		else
		{
			printf("No bones are found in the skeleton '%s'\n", skeleton->getName());
			return false;
		}
		
		return true;
	}
	else
	{
		printf("No skeletons found in the animation database '%s'. Requires at least 1\n", animDB->getName());
		return false;
	}
}

igBool ExtractMotionRaven::applyInfo(igInfo* info)
{
	return info->isOfType(igAnimationDatabase::getClassMeta()) && ApplyToDB(static_cast<igAnimationDatabase*>(info));
}

igBool ExtractMotionRaven::canOptimize(igInfo* info)
{
	return info->isOfType(igAnimationDatabase::getClassMeta());
}

}

}