/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "SkeletalSprite.h"
#include "Particles.h"
#include "MathFunctions.h"

#include "tinyxml2.h"
using namespace tinyxml2;

std::string SkeletalSprite::animationPath				= "data/animations/";
std::string SkeletalSprite::skinPath					= "skins/";

std::string SkeletalSprite::secondaryAnimationPath		= "";

static std::map<std::string, XMLDocument*> skelCache;

static XMLDocument *_retrieveSkeletalXML(const std::string& name, bool keepEmpty)
{
	auto it = skelCache.find(name);
	if(it != skelCache.end())
		return it->second;

	XMLDocument *doc = readXML(name, NULL, keepEmpty);
	if(doc)
		skelCache[name] = doc;

	return doc;
}

void SkeletalSprite::clearCache()
{
	for(auto& i: skelCache) delete i.second;
	skelCache.clear();
}


void SkeletalKeyframe::copyAllButTime(SkeletalKeyframe *copy)
{
	if (!copy) return;

	float t = this->t;
	(*this) = (*copy);
	this->t = t;
}

Bone::Bone() : Quad()
{
	addType(SCO_BONE);
	fileRenderQuad = true;
	skeleton = 0;
	generateCollisionMask = true;
	animated = ANIM_ALL;
	originalScale = Vector(1,1);
	boneIdx = pidx = -1;
	rbp = 0;
	segmentChain = 0;

	minDist = maxDist = 128;
	reverse = false;
	selectable = true;
	originalRenderPass = 0;
}


void Bone::destroy()
{
	Quad::destroy();

	for (auto& seg: segments)
	{
		seg->setLife(1.0);
		seg->setDecayRate(10);
		seg->alpha = 0;
	}
	segments.clear();
}

void Bone::addSegment(Bone *b)
{
	segments.push_back(b);

	b->segmentChain = 2;

	skeleton->removeChild(b);

	core->getTopStateData()->addRenderObject(b, skeleton->getTopLayer());
	b->position = this->getWorldPosition();
}

void Bone::createStrip(bool vert, int num)
{
	if (!vert)
	{
		createGrid(num, 2);
	}
	else
	{
		createGrid(2, num);
	}
	stripVert = vert;
	gridType = GRID_SET;
	changeStrip.resize(num);
	setGridPoints(vert, strip);
}

Quad* Bone::addFrame(const std::string &gfx)
{
	renderQuad = false;
	Quad *q = new Quad();
	q->setTexture(gfx);
	q->renderBeforeParent = 1;
	addChild(q, PM_POINTER);
	return q;
}

void Bone::showFrame(int idx)
{
	for (auto& r: children) r->alpha = 0;
	children[idx]->alpha = 1; //! check
}

void Bone::setAnimated(int b)
{
	animated = b;
}


void Bone::setSegmentProps(int minDist, int maxDist, bool reverse)
{
	this->minDist = minDist;
	this->maxDist = maxDist;
	this->reverse = reverse;
}

void Bone::updateSegment(Bone *b, const Vector &diff)
{


	float angle = -1;
	if (diff.getSquaredLength2D() > sqr(maxDist))
	{
		Vector useDiff = diff;
		useDiff.setLength2D(maxDist);
		Vector reallyUseDiff = diff - useDiff;
		b->position += reallyUseDiff;

		MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), diff, angle);
	}
	else if (diff.getSquaredLength2D() > sqr(minDist))
	{
		b->position += diff*0.05f;

		MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), diff, angle);


	}
	if (angle != -1)
	{


		if (b->rotation.z >= 270 && angle < 90)
		{
			b->rotation.stop();
			b->rotation.z -= 360;
		}

		if (b->rotation.z <= 90 && angle > 270)
		{
			b->rotation.stop();
			b->rotation.z += 360;
		}


		b->rotation.interpolateTo(Vector(0,0,angle),0.2);
	}

}

void Bone::updateSegments()
{
	if (segmentChain>0 && !segments.empty())
	{



		if (!reverse)
		{
			for (unsigned i = 0; i < segments.size(); i++)
			{
				Vector diff;
				if (i == 0)
				{
					Vector world = getWorldCollidePosition(segmentOffset);
					diff = world - segments[i]->getWorldPosition();
				}
				else
					diff = segments[i-1]->getWorldPosition() - segments[i]->getWorldPosition();

				updateSegment(segments[i], diff);
			}
		}
		else
		{
			int top = segments.size()-1;
			for (int i = top; i >= 0; i--)
			{
				Vector diff;
				if (i == top)
				{
					Vector world = getWorldCollidePosition(segmentOffset);
					diff = world - segments[i]->getWorldPosition();
				}
				else
					diff = segments[i+1]->getWorldPosition() - segments[i]->getWorldPosition();

				updateSegment(segments[i], diff);
			}
		}
	}
}

bool BoneCommand::parse(Bone *b, SimpleIStringStream &is)
{
	std::string type;
	is >> type;
	this->b = b;
	if (type=="AC_PRT_LOAD")
	{
		command = AC_PRT_LOAD;
		is >> slot >> file;
	}
	else if (type=="AC_SND_PLAY")
	{
		command = AC_SND_PLAY;
		is >> file;
	}
	else if (type=="AC_FRM_SHOW")
	{
		command = AC_FRM_SHOW;
		is >> slot;
	}
	else if (type=="AC_PRT_START")
	{
		command = AC_PRT_START;
		is >> slot;

	}
	else if (type=="AC_PRT_STOP")
	{
		command = AC_PRT_STOP;
		is >> slot;

	}
	else if (type=="AC_SEGS_START")
		command = AC_SEGS_START;
	else if (type=="AC_SEGS_STOP")
		command = AC_SEGS_STOP;
	else if (type == "AC_SET_PASS")
	{
		command = AC_SET_PASS;
		is >> slot;
	}
	else if(type == "AC_RESET_PASS")
		command = AC_RESET_PASS;
	else // fail
	{
		std::ostringstream os;
		os << "Failed to parse bone command string: invalid command: " << type;
		errorLog(os.str());
	}

	return true;
}

void BoneCommand::run()
{

	switch(command)
	{
	case AC_SND_PLAY:
	{
		core->sound->playSfx(file);
	}
	break;
	case AC_FRM_SHOW:
	{
		b->showFrame(slot);
	}
	break;
	case AC_PRT_LOAD:
	{
		ParticleEffect *e = b->emitters[slot];
		if (e)
		{
			e->load(file);
		}
	}
	break;
	case AC_PRT_START:
	{
		ParticleEffect *e = b->emitters[slot];
		if (e)
			e->start();
	}
	break;
	case AC_PRT_STOP:
	{
		ParticleEffect *e = b->emitters[slot];
		if (e)
			e->stop();
	}
	break;
	case AC_SET_PASS:
		b->setRenderPass(slot);
	break;
	case AC_RESET_PASS:
		b->setRenderPass(b->originalRenderPass);
	break;
	case AC_SEGS_START:
	case AC_SEGS_STOP:
		break;
	}
}


AnimationLayer::AnimationLayer()
{
	lastNewKey = 0;
	fallThru= 0;

	timer = 0;
	loop = 0;
	enqueuedAnimationLoop = 0;
	timeMultiplier = 1;
	animationLength = 0;
	currentAnimation = 0;
	animating = false;
	fallThruSpeed = 0;
	s = 0;
}

void AnimationLayer::setTimeMultiplier(float t)
{
	timeMultiplier = t;
}

void AnimationLayer::playCurrentAnimation(int loop)
{
	playAnimation(currentAnimation, loop);
}

void AnimationLayer::animate(const std::string &a, int loop)
{
	std::string animation = a;
	stringToLower(animation);

	bool played = false;
	for (unsigned i = 0; i < s->animations.size(); i++)
	{
		if (s->animations[i].name == animation)
		{
			playAnimation(i, loop);
			played = true;
			break;
		}
	}
	if (!played)
	{
		std::ostringstream os;
		os << "Could not find animation: " << animation;
		debugLog(os.str());
	}
}

void AnimationLayer::playAnimation(int idx, int loop)
{
	if (!(&s->animLayers[0] == this))
	{
		fallThru = 1;
		fallThruSpeed = 10;
	}
	timeMultiplier = 1;

	currentAnimation = idx;
	timer = 0;
	animating = true;

	this->loop = loop;

	animationLength = getCurrentAnimation()->getAnimationLength();

}

void AnimationLayer::enqueueAnimation(const std::string& anim, int loop)
{
	enqueuedAnimation = anim;
	enqueuedAnimationLoop = loop;
	stringToLower(enqueuedAnimation);
}

float AnimationLayer::transitionAnimate(std::string anim, float time, int loop)
{
	stringToLower(anim);
	float totalTime =0;
	if (createTransitionAnimation(anim, time))
	{
		timeMultiplier = 1;

		currentAnimation = -1;
		this->loop = 0;
		timer = 0;
		animating = 1;
		animationLength = getCurrentAnimation()->getAnimationLength();
		enqueueAnimation(anim, loop);
		Animation *a = this->s->getAnimation(anim);
		if (a)
		{
			if (loop > -1)
				totalTime = a->getAnimationLength()*(loop+1) + time;
			else
				totalTime = a->getAnimationLength() + time;
		}
	}
	return totalTime;
}

void AnimationLayer::setSkeletalSprite(SkeletalSprite *s)
{
	this->s = s;
}

Animation* AnimationLayer::getCurrentAnimation()
{
	if (currentAnimation == -1)
		return &blendAnimation;
	if (currentAnimation < 0 || currentAnimation >= s->animations.size())
	{
		std::ostringstream os;
		os << "skel: " << s->filenameLoaded << " currentAnimation: " << currentAnimation << " is out of range\n error in anim file?";
		exit_error(os.str());
		return 0;
	}
	return &s->animations[currentAnimation];
}

bool AnimationLayer::createTransitionAnimation(const std::string& anim, float time)
{

	Animation *to = s->getAnimation(anim);
	if (!to) return false;
	blendAnimation.keyframes.clear();
	SkeletalKeyframe k;
	k.t = 0;
	for (auto& bone: s->bones)
	{
		BoneKeyframe b;
		b.idx = bone->boneIdx;
		b.x = bone->position.x;
		b.y = bone->position.y;
		b.rot = bone->rotation.z;
		b.strip = bone->strip;
		b.sx = bone->scale.x;
		b.sy = bone->scale.y;
		k.keyframes.push_back(b);
	}
	blendAnimation.keyframes.push_back(k);

	SkeletalKeyframe k2;
	SkeletalKeyframe *rk = to->getKeyframe(0);
	if (!rk) return false;
	k2 = *rk;
	k2.t = time;
	blendAnimation.keyframes.push_back(k2);

	blendAnimation.name = anim;
	return true;
}


void AnimationLayer::stopAnimation()
{
	if(s->loaded && getCurrentAnimation()->resetPassOnEnd)
		resetPass();
	animating = false;
	if (!enqueuedAnimation.empty())
	{
		animate(enqueuedAnimation, enqueuedAnimationLoop);
		enqueuedAnimation = "";
		enqueuedAnimationLoop = 0;
	}
}

bool AnimationLayer::isAnimating()
{
	return animating;
}

float AnimationLayer::getAnimationLength()
{
	return animationLength;
}

Animation::Animation()
: resetPassOnEnd(false)
{
}

int Animation::getNumKeyframes()
{
	return keyframes.size();
}

SkeletalKeyframe *Animation::getKeyframe(int key)
{
	if (key < 0 || key >= keyframes.size()) return 0;
	return &keyframes[key];
}

void Animation::reverse()
{
	Keyframes copy = keyframes;
	Keyframes copy2 = keyframes;
	keyframes.clear();
	int sz = copy.size()-1;
	for (int i = sz; i >= 0; i--)
	{
		keyframes.push_back(copy[i]);
		keyframes[keyframes.size()-1].t = copy2[sz-i].t;
	}
	reorderKeyframes();
}

float Animation::getAnimationLength()
{
	return getLastKeyframe()->t;
}

SkeletalKeyframe *Animation::getLastKeyframe()
{
	if (!keyframes.empty())
		return &keyframes[keyframes.size()-1];
	return 0;
}

SkeletalKeyframe *Animation::getFirstKeyframe()
{
	if (!keyframes.empty())
		return &keyframes[0];
	return 0;
}

void Animation::reorderKeyframes()
{

	for (unsigned i = 0; i < keyframes.size(); i++)
	{
		for (unsigned j = 0; j < keyframes.size()-1; j++)
		{
			if (keyframes[j].t > keyframes[j+1].t)
			{
				SkeletalKeyframe temp = keyframes[j+1];
				keyframes[j+1] = keyframes[j];
				keyframes[j] = temp;
			}
		}
	}
}

void Animation::cloneKey(int key, float toffset)
{
	std::vector<SkeletalKeyframe> copy = this->keyframes;
	keyframes.clear();
	int i = 0;
	for (i = 0; i <= key; i++)
		keyframes.push_back(copy[i]);
	for (i = key; i < copy.size(); i++)
		keyframes.push_back(copy[i]);
	keyframes[key+1].t += toffset;
}

void Animation::deleteKey(int key)
{
	std::vector<SkeletalKeyframe> copy = this->keyframes;
	keyframes.clear();
	int i = 0;
	for (i = 0; i < key; i++)
		keyframes.push_back(copy[i]);
	for (i = key+1; i < copy.size(); i++)
		keyframes.push_back(copy[i]);
}

int Animation::getSkeletalKeyframeIndex(SkeletalKeyframe *skey)
{
	for (unsigned i = 0; i < keyframes.size(); i++)
	{
		if (&keyframes[i] == skey) return i;
	}
	return -1;
}

BoneKeyframe *SkeletalKeyframe::getBoneKeyframe(int idx)
{
	for (auto& kf: keyframes)
	{
		if (kf.idx == idx) return &kf;
	}
	return 0;
}

SkeletalKeyframe *Animation::getPrevKeyframe(float t)
{
	int kf = -1;
	for (int i = keyframes.size()-1; i >= 0; i--)
	{
		if (t >= keyframes[i].t)
		{
			kf = i;
			break;
		}
	}
	if (kf == -1) return 0;
	return &keyframes[kf];
}

SkeletalKeyframe *Animation::getNextKeyframe(float t)
{
	for (auto& kf: keyframes)
	{
		if (t <= kf.t) return &kf;
	}
	return 0;
}

SkeletalSprite::SkeletalSprite() : RenderObject()
{
	frozen = false;
	animKeyNotify = 0;
	loaded = false;
	animLayers.resize(10);
	for (auto& layer: animLayers) layer.setSkeletalSprite(this);
	selectedBone = -1;
}

void SkeletalSprite::setAnimationKeyNotify(RenderObject *r)
{
	animKeyNotify = r;
}

void SkeletalSprite::animate(const std::string &animation, int loop, int layer)
{
	animLayers[layer].animate(animation, loop);
}

float SkeletalSprite::transitionAnimate(const std::string& anim, float time, int loop, int layer)
{
	AnimationLayer *animLayer = getAnimationLayer(layer);
	if (animLayer)
		return animLayer->transitionAnimate(anim, time, loop);

	std::ostringstream os;
	os << "playing animation on invalid layer: " << layer;
	errorLog(os.str());
	return 0;
}

AnimationLayer* SkeletalSprite::getAnimationLayer(int l)
{
	if (l >= 0 && l < animLayers.size())
	{
		return &animLayers[l];
	}
	std::ostringstream os;
	os << "couldn't get animLayer: " << l;
	debugLog(os.str());
	return 0;
}

bool SkeletalSprite::isLoaded()
{
	return loaded;
}

void SkeletalSprite::onUpdate(float dt)
{
	if (frozen) return;
	RenderObject::onUpdate(dt);

	for (auto& b: bones)
	{
		if (b && !b->collisionMask.empty())
		{
			if (b->collisionMask.size() != b->transformedCollisionMask.size())
			{
				b->transformedCollisionMask.resize(b->collisionMask.size());
			}
			for (unsigned i = 0; i < b->collisionMask.size(); i++)
			{
				b->transformedCollisionMask[i] = b->getWorldCollidePosition(b->collisionMask[i]);
			}
		}
	}

	for (auto& layer: animLayers) layer.update(dt);
}

void AnimationLayer::update(float dt)
{
	timeMultiplier.update(dt);
	if (animating)
	{
		timer += dt*timeMultiplier.x;

		if (timer > animationLength)
		{
			float leftover;
			if (animationLength > 0)
				leftover = fmodf(timer, animationLength);
			else
				leftover = 0;
			timer = animationLength;
			if (loop==-1 || loop > 0)
			{
				playAnimation(this->currentAnimation, loop);
				if (loop > 0)
					loop --;
				timer = leftover;
			}
			else
			{
				stopAnimation();
			}
		}
		updateBones();
	}
	else if (!animating)
	{
		if (fallThru > 0)
		{
			fallThru -= dt * fallThruSpeed;
			if (fallThru < 0)
				fallThru = 0;
			updateBones();
		}
	}
}

bool SkeletalSprite::saveSkeletal(const std::string &fn)
{
	std::string file, filename=fn;
	stringToLower(filename);

	if (!secondaryAnimationPath.empty())
	{
		createDir(secondaryAnimationPath);
		file = secondaryAnimationPath + filename + ".xml";
	}
	else
	{
		file = animationPath + filename + ".xml";
	}

	int i = 0;
	XMLDocument *xml = _retrieveSkeletalXML(file, true);
	xml->Clear();

	XMLElement *animationLayers = xml->NewElement("AnimationLayers");
	for (auto& layer: animLayers)
	{
		XMLElement *animationLayer = xml->NewElement("AnimationLayer");
		if (layer.ignoreBones.size() > 0)
		{
			std::ostringstream os;
			for (auto& bone: layer.ignoreBones) os << bone << " ";
			animationLayer->SetAttribute("ignore", os.str().c_str());
		}
		if (layer.includeBones.size() > 0)
		{
			std::ostringstream os;
			for (auto& bone: layer.includeBones) os << bone << " ";
			animationLayer->SetAttribute("include", os.str().c_str());
		}
		if (!layer.name.empty())
		{
			animationLayer->SetAttribute("name", layer.name.c_str());
		}

		animationLayers->InsertEndChild(animationLayer);
	}
	xml->InsertEndChild(animationLayers);


	XMLElement *bones = xml->NewElement("Bones");
	for (auto& b: this->bones)
	{
		XMLElement *bone = xml->NewElement("Bone");
		bone->SetAttribute("idx", b->boneIdx);
		bone->SetAttribute("gfx", b->gfx.c_str());
		bone->SetAttribute("pidx", b->pidx);
		bone->SetAttribute("name", b->name.c_str());
		bone->SetAttribute("fh", b->isfh());
		bone->SetAttribute("fv", b->isfv());
		bone->SetAttribute("gc", b->generateCollisionMask);
		bone->SetAttribute("cr", b->collideRadius);
		if (!b->renderQuad)
		{
			bone->SetAttribute("rq", b->fileRenderQuad);
		}
		if (!b->selectable)
		{
			bone->SetAttribute("sel", b->selectable);
		}
		if (b->rbp) bone->SetAttribute("rbp", b->rbp);
		if (b->originalRenderPass) bone->SetAttribute("pass", b->originalRenderPass);
		if (b->offset.x) bone->SetAttribute("offx", b->offset.x);
		if (b->offset.y)
			bone->SetAttribute("offy", b->offset.y);
		if (!b->prt.empty())
			bone->SetAttribute("prt", b->prt.c_str());
		if (!b->changeStrip.empty())
		{
			std::ostringstream os;
			os << b->stripVert << " " << b->changeStrip.size();
			bone->SetAttribute("strip", os.str().c_str());
		}
		if (!b->internalOffset.isZero())
		{
			std::ostringstream os;
			os << b->internalOffset.x << " " << b->internalOffset.y;
			bone->SetAttribute("io", os.str().c_str());
		}
		if (b->isRepeatingTextureToFill())
		{
			bone->SetAttribute("rt", 1);
		}
		if (b->originalScale.x != 1 || b->originalScale.y != 1)
		{
			std::ostringstream os;
			os << b->originalScale.x << " " << b->originalScale.y;
			bone->SetAttribute("sz", os.str().c_str());
		}


		for (auto& j:b->children)
		{
			Bone *b = dynamic_cast<Bone*>(j);
			Quad *q = dynamic_cast<Quad*>(j);
			Particle *p = dynamic_cast<Particle*>(j);
			if (q && !b && !p)
			{
				XMLElement *frame = xml->NewElement("Frame");
				frame->SetAttribute("gfx", q->texture->name.c_str());
				if (q->getRenderPass() != 0)
				{
					frame->SetAttribute("pass", q->getRenderPass());
				}
				bone->InsertEndChild(frame);
			}
		}
		bones->InsertEndChild(bone);
	}
	xml->InsertEndChild(bones);

	XMLElement *animations = xml->NewElement("Animations");
	for (auto& a: this->animations)
	{
		XMLElement *animation = xml->NewElement("Animation");
		animation->SetAttribute("name", a.name.c_str());
		if(a.resetPassOnEnd)
			animation->SetAttribute("resetPassOnEnd", a.resetPassOnEnd);
		for (auto& kf: a.keyframes)
		{
			XMLElement *key = xml->NewElement("Key");
			if (!kf.sound.empty())
				key->SetAttribute("sound", kf.sound.c_str());
			if (!kf.cmd.empty())
			{
				key->SetAttribute("cmd", kf.cmd.c_str());
			}
			if (kf.lerpType != 0)
			{
				key->SetAttribute("lerp", kf.lerpType);
			}
			std::ostringstream os;
			os << kf.t << " ";
			std::ostringstream szos;
			for (auto& b: kf.keyframes)
			{
				os << b.idx << " " << b.x << " " << b.y << " " << b.rot << " ";
				os << b.strip.size() << " ";
				for (auto& s: b.strip) os << s.x << " " << s.y << " ";
				if (b.doScale)
				{
					szos << b.idx << " " << b.sx << " " << b.sy << " ";
				}
			}
			std::string szoss = szos.str();
			if (!szoss.empty())
				key->SetAttribute("sz", szoss.c_str());

			key->SetAttribute("e", os.str().c_str());

			animation->InsertEndChild(key);
		}
		animations->InsertEndChild(animation);
	}
	xml->InsertEndChild(animations);
	return xml->SaveFile(file.c_str()) == XML_SUCCESS;
}

int SkeletalSprite::getBoneIdx(Bone *b)
{
	for (int i = 0; i < bones.size(); i++)
	{
		if (bones[i] == b)
			return i;
	}
	return -1;
}

void SkeletalSprite::toggleBone(int idx, int v)
{
	if (idx >= 0 && idx < bones.size())
	{
		bones[idx]->alpha.x = v;
	}
}

Bone *SkeletalSprite::getBoneByName(const std::string &name)
{
	for (auto& bone: bones)
	{
		if (bone->name == name) return bone;
	}
	std::ostringstream os;
	os << "Could not find bone with name[" << name << "]";
	debugLog(os.str());
	return 0;
}

Bone *SkeletalSprite::getBoneByIdx(int idx)
{
	for (auto& bone: bones)
	{
		if (bone->boneIdx == idx) return bone;
	}
	std::ostringstream os;
	os << "Could not find bone with idx[" << idx << "]";
	debugLog(os.str());
	return 0;
}

Bone *SkeletalSprite::initBone(int idx, std::string gfx, int pidx, int rbp, std::string name, float cr, bool fh, bool fv)
{
	Bone *b = new Bone;
	b->boneIdx = idx;
	b->setTexture(gfx);
	b->skeleton = this;
	b->gfx = gfx;
	b->rbp = rbp;
	b->renderBeforeParent = rbp;
	b->pidx = pidx;
	b->collideRadius = cr;
	b->name = name;

	if (fh)
		b->flipHorizontal();
	if (fv)
		b->flipVertical();
	bones.push_back(b);
	return b;
}

void SkeletalSprite::firstAnimation()
{
	stopAnimation();
	animLayers[0].currentAnimation = 0;
}

void SkeletalSprite::lastAnimation()
{
	stopAnimation();
	animLayers[0].currentAnimation = animations.size()-1;
}

void SkeletalSprite::nextAnimation()
{
	stopAnimation();
	animLayers[0].currentAnimation++;
	if (animLayers[0].currentAnimation >= animations.size())
		animLayers[0].currentAnimation = 0;
}

void SkeletalSprite::prevAnimation()
{
	stopAnimation();
	animLayers[0].currentAnimation--;
	if (animLayers[0].currentAnimation < 0)
		animLayers[0].currentAnimation = animations.size()-1;
}

void SkeletalSprite::deleteBones()
{
	bones.clear();
	for(auto& child: children) child->safeKill();
}

Animation *SkeletalSprite::getAnimation(const std::string& anim)
{
	for (auto& a: animations)
	{
		if (a.name == anim) return &a;
	}
	return 0;
}

void SkeletalSprite::loadSkin(const std::string &fn)
{
	std::string file;

	if (!secondaryAnimationPath.empty())
	{
		file = secondaryAnimationPath + skinPath + fn + ".xml";
	}

	if (file.empty() || !exists(file, false))
	{
		file = animationPath + skinPath + fn + ".xml";
	}

	file = core->adjustFilenameCase(file);

	if (!exists(file))
	{
		errorLog("Could not load skin[" + file + "] - File not found.");
		return;
	}
	XMLDocument *d = _retrieveSkeletalXML(file, false);
	if(!d)
	{
		errorLog("Could not load skin[" + file + "] - Malformed XML.");
		return;
	}

	XMLElement *bonesXml = d->FirstChildElement("Bones");
	if (bonesXml)
	{
		XMLElement *boneXml = bonesXml->FirstChildElement("Bone");
		while (boneXml)
		{
			int idx = atoi(boneXml->Attribute("idx"));
			Bone *b = getBoneByIdx(idx);
			if (b)
			{
				if (boneXml->Attribute("rq"))
				{
					int rq = atoi(boneXml->Attribute("rq"));
					b->renderQuad = rq;
				}

				std::string gfx;
				if (boneXml->Attribute("gfx"))
				{
					gfx = boneXml->Attribute("gfx");
					if (!gfx.empty())
					{
						b->gfx = gfx;
						b->setTexture(b->gfx);
						b->renderQuad = true;
					}
				}

				if (gfx.empty())
				{
					b->renderQuad = false;
				}

				if (boneXml->Attribute("fh"))
				{
					int fh = atoi(boneXml->Attribute("fh"));
					if (fh)
						b->flipHorizontal();
				}
				if (boneXml->Attribute("fv"))
				{
					int fv = atoi(boneXml->Attribute("fv"));
					if (fv)
						b->flipVertical();
				}



			}
			else
			{
				std::ostringstream os;
				os << "SkinLoad: Could not find idx[" << idx << "]";
				debugLog(os.str());
			}
			boneXml = boneXml->NextSiblingElement("Bone");
		}
	}
}

void SkeletalSprite::stopAnimation(int layer)
{
	animLayers[layer].stopAnimation();
}

void SkeletalSprite::stopAllAnimations()
{
	for (auto& layer: animLayers) layer.stopAnimation();
}

void SkeletalSprite::playCurrentAnimation(int loop, int layer)
{
	animLayers[layer].playCurrentAnimation(loop);
}

void SkeletalSprite::loadSkeletal(const std::string &fn)
{
	filenameLoaded = "";
	loaded = false;
	stopAnimation();
	animLayers.clear();
	deleteBones();
	filenameLoaded = fn;
	stringToLower(filenameLoaded);

	std::string file;

	if (!secondaryAnimationPath.empty())
	{
		file = secondaryAnimationPath + filenameLoaded + ".xml";
	}

	if (file.empty() || !exists(file, false))
	{
		file = animationPath + filenameLoaded + ".xml";
	}

	if (!exists(file))
	{
		filenameLoaded = "";
		errorLog("Could not load skeletal[" + file + "] - File not found.");
		return;
	}

	file = core->adjustFilenameCase(file);

	XMLDocument *xml = _retrieveSkeletalXML(file, false);
	if(!xml)
	{
		filenameLoaded = "";
		errorLog("Could not load skeletal[" + file + "] - Malformed XML.");
		return;
	}

	loaded = true;

	XMLElement *bones = xml->FirstChildElement("Bones");
	if (bones)
	{
		if (bones->Attribute("scale"))
		{
			SimpleIStringStream is(bones->Attribute("scale"));
			is >> scale.x >> scale.y;
		}

		XMLElement *bone = bones->FirstChildElement("Bone");
		while(bone)
		{
			int idx = atoi(bone->Attribute("idx"));
			int pidx = -1, rbp=0, cr=0, fh=0, fv=0;

			std::string name;
			if (bone->Attribute("pidx"))
				pidx = atoi(bone->Attribute("pidx"));
			if (bone->Attribute("rbp"))
				rbp = atoi(bone->Attribute("rbp"));

			if (bone->Attribute("name"))
				name = bone->Attribute("name");
			if (bone->Attribute("cr"))
				cr = atoi(bone->Attribute("cr"));
			if (bone->Attribute("fh"))
				fh = atoi(bone->Attribute("fh"));
			if (bone->Attribute("fv"))
				fv = atoi(bone->Attribute("fv"));

			std::string gfx = bone->Attribute("gfx");
			Bone *newb = initBone(idx, gfx, pidx, rbp, name, cr, fh, fv);
			if (bone->Attribute("offx"))
				newb->offset.x = atoi(bone->Attribute("offx"));
			if (bone->Attribute("offy"))
				newb->offset.y = atoi(bone->Attribute("offy"));

			if (bone->Attribute("prt"))
			{
				newb->prt = bone->Attribute("prt");
				SimpleIStringStream is(newb->prt);
				int slot;
				while (is >> slot)
				{
					std::string pfile;
					is >> pfile;
					// add particle system + load
					newb->emitters[slot] = new ParticleEffect;
					ParticleEffect *e = newb->emitters[slot];
					newb->addChild(e, PM_POINTER);
					e->load(pfile);
					// hack for now:
					//e->start();
				}
			}
			XMLElement *fr=0;
			fr = bone->FirstChildElement("Frame");
			int frc=0;
			while(fr)
			{
				Quad *q=0;
				std::string gfx;
				if (fr->Attribute("gfx"))
				{
					gfx = fr->Attribute("gfx");
					q = newb->addFrame(gfx);
				}
				if (fr->Attribute("pass"))
				{
					if (q)
					{
						q->setRenderPass(atoi(fr->Attribute("pass")));
					}
				}
				fr = fr->NextSiblingElement("Frame");
				frc++;
			}
			if (frc)
			{
				newb->showFrame(0);
			}
			if (bone->Attribute("pass"))
			{
				int pass = atoi(bone->Attribute("pass"));
				newb->originalRenderPass = pass;
				newb->setRenderPass(pass);
			}
			if (bone->Attribute("gc"))
			{
				newb->generateCollisionMask = atoi(bone->Attribute("gc"));
			}
			if (bone->Attribute("rq"))
			{
				newb->renderQuad = newb->fileRenderQuad = atoi(bone->Attribute("rq"));
			}
			if (bone->Attribute("io"))
			{
				SimpleIStringStream is(bone->Attribute("io"));
				is >> newb->internalOffset.x >> newb->internalOffset.y;
			}

			if (bone->Attribute("strip"))
			{
				SimpleIStringStream is(bone->Attribute("strip"));
				bool vert;
				int num;
				is >> vert >> num;
				newb->createStrip(vert, num);
			}
			if (bone->Attribute("sz"))
			{
				float sx, sy;
				SimpleIStringStream is(bone->Attribute("sz"));
				is >> sx >> sy;

				newb->scale = newb->originalScale = Vector(sx,sy);
			}
			if (bone->Attribute("rt"))
			{
				newb->repeatTextureToFill(true);
			}

			if (bone->Attribute("blend"))
			{
				//if (bone->Attribute("blend")=="add")
				newb->blendType = blendType = BLEND_ADD;
			}

			if (bone->Attribute("alpha"))
			{
				float a=1.0;
				SimpleIStringStream is(bone->Attribute("alpha"));
				is >> a;
				newb->alpha = a;
			}

			if (bone->Attribute("alphaMod"))
			{
				float a=1.0;
				SimpleIStringStream is(bone->Attribute("alphaMod"));
				is >> a;
				newb->alphaMod = a;
			}

			if (bone->Attribute("segs"))
			{
				int x, y;
				float dgox, dgoy, dgmx, dgmy, dgtm;
				bool dgo;
				SimpleIStringStream is(bone->Attribute("segs"));
				is >> x >> y >> dgox >> dgoy >> dgmx >> dgmy >> dgtm >> dgo;
				newb->setSegs(x, y, dgox, dgoy, dgmx, dgmy, dgtm, dgo);
			}

			if (bone->Attribute("color"))
			{
				SimpleIStringStream in(bone->Attribute("color"));
				in >> newb->color.x >> newb->color.y >> newb->color.z;
			}
			if (bone->Attribute("sel"))
			{
				newb->selectable = bone->BoolAttribute("sel");
			}
			bone = bone->NextSiblingElement("Bone");
		}
		// attach bones
		for (auto& b: this->bones)
		{
			if (b->pidx != -1)
			{
				Bone *pb = getBoneByIdx(b->pidx);
				if (!pb)
				{
					std::ostringstream os;
					os << "Parent bone not found, index: " << b->pidx << " from bone idx: " << b->getIdx();
					debugLog(os.str());
				}
				else pb->addChild(b, PM_POINTER);
			}
			else addChild(b, PM_POINTER);
		}
	}

	animLayers.clear();
	XMLElement *animationLayers = xml->FirstChildElement("AnimationLayers");
	if (animationLayers)
	{
		XMLElement *animationLayer = animationLayers->FirstChildElement("AnimationLayer");
		while (animationLayer)
		{
			AnimationLayer newAnimationLayer;
			if (animationLayer->Attribute("ignore"))
			{
				SimpleIStringStream is(animationLayer->Attribute("ignore"));
				int t;
				while (is >> t)
				{
					newAnimationLayer.ignoreBones.push_back(t);
				}
			}
			if (animationLayer->Attribute("include"))
			{
				SimpleIStringStream is(animationLayer->Attribute("include"));
				int t;
				while (is >> t)
				{
					newAnimationLayer.includeBones.push_back(t);
				}
			}
			if (animationLayer->Attribute("name"))
			{
				newAnimationLayer.name = animationLayer->Attribute("name");
			}
			newAnimationLayer.setSkeletalSprite(this);
			animLayers.push_back(newAnimationLayer);
			animationLayer = animationLayer->NextSiblingElement("AnimationLayer");
		}
	}

	animations.clear();
	XMLElement *animations = xml->FirstChildElement("Animations");
	if (animations)
	{
		XMLElement *animation = animations->FirstChildElement("Animation");
		while(animation)
		{
			Animation newAnimation;
			newAnimation.name = animation->Attribute("name");
			newAnimation.resetPassOnEnd = animation->BoolAttribute("resetPassOnEnd");
			stringToLower(newAnimation.name);

			XMLElement *key = animation->FirstChildElement("Key");
			while (key)
			{
				SkeletalKeyframe newSkeletalKeyframe;
				if (key->Attribute("e"))
				{
					float time;
					SimpleIStringStream is(key->Attribute("e"));
					is >> time;
					int idx, x, y, rot, strip;
					newSkeletalKeyframe.t = time;
					if (key->Attribute("sound"))
					{
						newSkeletalKeyframe.sound = key->Attribute("sound");
					}
					if (key->Attribute("lerp"))
					{
						newSkeletalKeyframe.lerpType = atoi(key->Attribute("lerp"));
					}
					while (is >> idx)
					{
						BoneKeyframe b;
						is >> x >> y >> rot >> strip;
						b.idx = idx;
						b.x = x;
						b.y = y;
						b.rot = rot;
						if (strip > 0)
						{
							b.strip.resize(strip);
							for (auto& i: b.strip) is >> i.x >> i.y;
						}
						if (key->Attribute("sz"))
						{
							SimpleIStringStream is2(key->Attribute("sz"));
							int midx;
							float bsx, bsy;
							while (is2 >> midx)
							{
								is2 >> bsx >> bsy;
								if (midx == idx)
								{
									b.doScale = true;
									b.sx = bsx;
									b.sy = bsy;
									break;
								}
							}
						}
						newSkeletalKeyframe.keyframes.push_back(b);
					}

				}
				if (key->Attribute("d"))
				{
					float time;
					SimpleIStringStream is(key->Attribute("d"));
					is >> time;
					int idx, x, y, rot;

					newSkeletalKeyframe.t = time;
					if (key->Attribute("sound"))
					{
						newSkeletalKeyframe.sound = key->Attribute("sound");
					}
					while (is >> idx)
					{
						is >> x >> y >> rot;
						BoneKeyframe b;
						b.idx = idx;
						b.x = x;
						b.y = y;
						b.rot = rot;
						newSkeletalKeyframe.keyframes.push_back(b);
					}
				}
				if (key->Attribute("cmd"))
				{
					newSkeletalKeyframe.cmd = key->Attribute("cmd");
					SimpleIStringStream is(newSkeletalKeyframe.cmd);
					int bidx;
					while (is >> bidx)
					{
						Bone *b = this->getBoneByIdx(bidx);
						if (b)
						{
							BoneCommand bcmd;
							if(!bcmd.parse(b, is))
								break;
							newSkeletalKeyframe.commands.push_back(bcmd);
						}
						else
						{
							std::ostringstream os;
							os << "SkeletalSprite::loadSkeletal: File " << fn << " anim " << newAnimation.name << " specifies non-existing bone idx " << bidx;
							errorLog(os.str());
						}
					}
				}
				// generate empty bone keys
				for (auto& bone: this->bones)
				{
					if (!newSkeletalKeyframe.getBoneKeyframe(bone->boneIdx))
					{
						BoneKeyframe b;
						b.idx = bone->boneIdx;
						newSkeletalKeyframe.keyframes.push_back(b);
					}
				}
				newAnimation.keyframes.push_back(newSkeletalKeyframe);
				key = key->NextSiblingElement("Key");
			}
			animation = animation->NextSiblingElement("Animation");
			this->animations.push_back(newAnimation);
		}
	}
}

Animation *SkeletalSprite::getCurrentAnimation(int layer)
{
	return layer < animLayers.size() ? animLayers[layer].getCurrentAnimation() : NULL;
}

void SkeletalSprite::setTime(float time, int layer)
{
	if(layer < animLayers.size())
		animLayers[layer].timer = time;
}

void AnimationLayer::resetPass()
{
	for (auto& b: s->bones)
	{
		if (contains(b)) b->setRenderPass(b->originalRenderPass);
	}
}

bool AnimationLayer::contains(const Bone *b) const
{
	const int idx = b->boneIdx;
	if (!ignoreBones.empty())
	{
		for (int j = 0; j < ignoreBones.size(); j++)
			if (idx == ignoreBones[j])
				return false;
	}
	else if (!includeBones.empty())
	{
		for (int j = 0; j < includeBones.size(); j++)
			if (idx == includeBones[j])
				return true;
		return false;
	}

	return true;
}

void AnimationLayer::updateBones()
{
	if (!animating && !(&s->animLayers[0] == this) && fallThru == 0) return;

	SkeletalKeyframe *key1 = getCurrentAnimation()->getPrevKeyframe(timer);
	SkeletalKeyframe *key2 = getCurrentAnimation()->getNextKeyframe(timer);
	if (!key1 || !key2) return;
	float t1 = key1->t;
	float t2 = key2->t;
	float diff = t2-t1;
	float dt;
	if (diff != 0)
		dt = (timer - t1)/(t2-t1);
	else
		dt = 0;

	if (lastNewKey != key2)
	{
		if (!key2->sound.empty())
		{
			core->sound->playSfx(key2->sound);
		}
		if (!key2->commands.empty())
		{
			for (auto& k: key2->commands) k.run();
		}
		if (s->animKeyNotify)
		{
			s->animKeyNotify->onAnimationKeyPassed(getCurrentAnimation()->getSkeletalKeyframeIndex(lastNewKey));
		}
	}
	lastNewKey = key2;

	for (auto& b: s->bones)
	{
		if (b->segmentChain == 1)
		{
			b->updateSegments();
		}
		if (b->segmentChain < 2)
		{
			if (b->animated != Bone::ANIM_NONE && contains(b))
			{
				int idx = b->boneIdx;
				BoneKeyframe *bkey1 = key1->getBoneKeyframe(idx);
				BoneKeyframe *bkey2 = key2->getBoneKeyframe(idx);
				if (bkey1 && bkey2)
				{
					if (!animating && fallThru > 0)
					{
						//HACK: TODO: fix this up nice like below
						Vector p = Vector((bkey2->x-bkey1->x)*dt+bkey1->x, (bkey2->y-bkey1->y)*dt+bkey1->y);
						float rot = (bkey2->rot - bkey1->rot)*dt + bkey1->rot;
						p = Vector((p.x-b->position.x)*fallThru+b->position.x, (p.y-b->position.y)*fallThru+b->position.y);
						rot = (rot-b->rotation.z)*fallThru + b->rotation.z;
						if (b->animated==Bone::ANIM_ALL || b->animated==Bone::ANIM_POS)
							b->position = p;
						if (b->animated==Bone::ANIM_ALL || b->animated==Bone::ANIM_ROT)
							b->rotation.z = rot;
					}
					else
					{
						int lerpType = key2->lerpType;
						//k(0)×(2u3-3u2+1) + k(1)×(3u2-2u3)
						if (b->animated==Bone::ANIM_ALL || b->animated==Bone::ANIM_POS)
						{
							b->position = Vector(lerp(bkey1->x, bkey2->x, dt, lerpType), lerp(bkey1->y, bkey2->y, dt, lerpType));
						}
						if (b->animated==Bone::ANIM_ALL || b->animated==Bone::ANIM_ROT)
						{
							b->rotation.z = lerp(bkey1->rot, bkey2->rot, dt, lerpType);
						}
						if (b->animated==Bone::ANIM_ALL && (bkey1->doScale || bkey2->doScale))
						{
							b->scale.x = lerp(bkey1->sx, bkey2->sx, dt, lerpType);
							b->scale.y = lerp(bkey1->sy, bkey2->sy, dt, lerpType);
						}
						if (b->animated==Bone::ANIM_ALL && !b->changeStrip.empty())
						{
							if (bkey2->strip.size() < b->changeStrip.size())
								bkey2->strip.resize(b->changeStrip.size());
							if (bkey1->strip.size() < b->changeStrip.size())
								bkey1->strip.resize(b->changeStrip.size());
							for (unsigned i = 0; i < b->changeStrip.size(); i++)
							{
								b->changeStrip[i] = Vector(lerp(bkey1->strip[i].x, bkey2->strip[i].x, dt, lerpType), lerp(bkey1->strip[i].y, bkey2->strip[i].y, dt, lerpType));
							}
							b->setGridPoints(b->stripVert, b->changeStrip);
						}
					}
				}
			}
		}
	}
}

void SkeletalSprite::setFreeze(bool f)
{
	frozen = f;
}

void SkeletalSprite::updateBones()
{
	if (!frozen)
	{
		for (auto& layer: animLayers) layer.updateBones();
	}
}

bool SkeletalSprite::isAnimating(int layer)
{
	return animLayers[layer].animating;
}

void SkeletalSprite::setTimeMultiplier(float t, int layer)
{
	animLayers[layer].timeMultiplier = t;
}

Bone* SkeletalSprite::getSelectedBone(bool mouseBased)
{
	if (!loaded) return 0;
	if (mouseBased)
	{
		float closestDist = HUGE_VALF;
		Bone *b = 0;
		Vector p = core->mouse.position;
		for (unsigned i = 0; i < bones.size(); i++)
		{
			if (bones[i]->renderQuad || core->getShiftState())
			{
				bones[i]->color = Vector(1,1,1);
				if (bones[i]->selectable && bones[i]->renderQuad && bones[i]->isCoordinateInsideWorld(p))
				{
					float dist = (bones[i]->getWorldPosition() - p).getSquaredLength2D();
					if (dist <= closestDist)
					{
						closestDist = dist;
						b = bones[i];
						selectedBone = i;
					}
				}
			}
		}
		if (b)
		{
			b->color = Vector(1,0,0);
		}
		return b;
	}
	// else
	if (!bones.empty() && selectedBone >= 0 && selectedBone < bones.size())
		return bones[selectedBone];

	return 0;
}


void SkeletalSprite::updateSelectedBoneColor()
{
	for (int i = 0; i < bones.size(); i++)
	{
		bones[i]->color = Vector(1,1,1);
	}
	Bone *b = bones[selectedBone];
	if (b)
		b->color = Vector(0.5,0.5,1);
}

void SkeletalSprite::setSelectedBone(int b)
{
	selectedBone = b;
	updateSelectedBoneColor();
}

void SkeletalSprite::selectPrevBone()
{
	const int oldsel = selectedBone;
	do
	{
		selectedBone++;
		if(selectedBone == oldsel)
			break;
		if (selectedBone >= bones.size())
			selectedBone = 0;
	}
	while (!bones[selectedBone]->selectable);
	updateSelectedBoneColor();
}

void SkeletalSprite::selectNextBone()
{
	const int oldsel = selectedBone;
	do
	{
		selectedBone--;
		if(selectedBone == oldsel)
			break;
		if (selectedBone < 0)
			selectedBone = bones.size()-1;
	}
	while (!bones[selectedBone]->selectable);
	updateSelectedBoneColor();
}


