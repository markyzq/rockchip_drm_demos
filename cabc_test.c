#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <xf86drm.h>
#include <xf86drmMode.h> 

#define FLAG_GAMMA 'a'
#define FLAG_MODE 'm'
#define FLAG_STAGE_UP 'u'
#define FLAG_STAGE_DOWN 'd'
#define FLAG_GLOBAL_DN 'g'
#define FLAG_CALC_PIXEL_NUM 'p'
#define FLAG_CRTC 'c'
#define FLAG_HELP 'h'
#define FLAG_LIST 'l'

struct object {
	int value;
	int id;
	int changed;
};

const char *const short_options = "hla:m:u:d:g:c:p:";
struct option long_options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "list", no_argument, NULL, 'l' },
	{ "crtc", required_argument, NULL, 'c' },
	{ "gamma", required_argument, NULL, 'a' },
	{ "mode", required_argument, NULL, 'm' },
	{ "stage_up", required_argument, NULL, 'u' },
	{ "stage_down", required_argument, NULL, 'd' },
	{ "global_dn", required_argument, NULL, 'g' },
	{ "calc_pixel_num", required_argument, NULL, 'p' },
	{ 0, 0, 0, 0},
};

/* dump_blob and dump_prop shamelessly copied from ../modetest/modetest.c */
static void
dump_blob(int fd, uint32_t blob_id)
{
	uint32_t i;
	unsigned char *blob_data;
	drmModePropertyBlobPtr blob;

	blob = drmModeGetPropertyBlob(fd, blob_id);
	if (!blob) {
		printf("\n");
		return;
	}

	blob_data = blob->data;

	for (i = 0; i < blob->length; i++) {
		if (i % 16 == 0)
			printf("\n\t\t\t");
		printf("%.2hhx", blob_data[i]);
	}
	printf("\n");

	drmModeFreePropertyBlob(blob);
}

static void
dump_prop(int fd, uint32_t prop_id, uint64_t value)
{
	int i;
	drmModePropertyPtr prop;

	prop = drmModeGetProperty(fd, prop_id);

	printf("\t%d", prop_id);
	if (!prop) {
		printf("\n");
		return;
	}

	printf(" %s:", prop->name);

	printf(" flags:");
	if (prop->flags & DRM_MODE_PROP_PENDING)
		printf(" pending");
	if (prop->flags & DRM_MODE_PROP_IMMUTABLE)
		printf(" immutable");
	if (drm_property_type_is(prop, DRM_MODE_PROP_SIGNED_RANGE))
		printf(" signed range");
	if (drm_property_type_is(prop, DRM_MODE_PROP_RANGE))
		printf(" range");
	if (drm_property_type_is(prop, DRM_MODE_PROP_ENUM))
		printf(" enum");
	if (drm_property_type_is(prop, DRM_MODE_PROP_BITMASK))
		printf(" bitmask");
	if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB))
		printf(" blob");
	if (drm_property_type_is(prop, DRM_MODE_PROP_OBJECT))
		printf(" object");
	printf("\n");

	if (drm_property_type_is(prop, DRM_MODE_PROP_RANGE)) {
		printf("\t\tvalues:");
		for (i = 0; i < prop->count_values; i++)
			printf(" %"PRIu64, prop->values[i]);
		printf("\n");
	}

	if (drm_property_type_is(prop, DRM_MODE_PROP_ENUM)) {
		printf("\t\tenums:");
		for (i = 0; i < prop->count_enums; i++)
			printf(" %s=%llu", prop->enums[i].name,
			       prop->enums[i].value);
		printf("\n");
	} else if (drm_property_type_is(prop, DRM_MODE_PROP_BITMASK)) {
		printf("\t\tvalues:");
		for (i = 0; i < prop->count_enums; i++)
			printf(" %s=0x%llx", prop->enums[i].name,
			       (1LL << prop->enums[i].value));
		printf("\n");
	} else {
		assert(prop->count_enums == 0);
	}

	if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB)) {
		printf("\t\tblobs:\n");
		for (i = 0; i < prop->count_blobs; i++)
			dump_blob(fd, prop->blob_ids[i]);
		printf("\n");
	} else {
		assert(prop->count_blobs == 0);
	}

	printf("\t\tvalue:");
	if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB))
		dump_blob(fd, value);
	else if (drm_property_type_is(prop, DRM_MODE_PROP_SIGNED_RANGE))
		printf(" %"PRId64"\n", value);
	else
		printf(" %"PRIu64"\n", value);

	drmModeFreeProperty(prop);
}
void help(void)
{
	printf(
"cabc test\n"
"command line options:\n\n"
"    -h or --help - this\n"
"    -a or --gamma=[float] - set pow(gamma) gamma table with gamma=f\n"
"    -m or --mode=[0 1 2 3] - cabc mode\n"
"                 0 - disabled cabc\n"
"                 1 - Predefine cabc normal mode\n"
"                 2 - Predefine cabc lowpower mode\n"
"                 3 - userspace mode\n"
"    -u or --stage_up=[0-511] - set cabc stage up\n"
"    -d or --stage_down=[0-255] - set cabc stage down\n"
"    -g or --global_dn=[0-255] - set cabc global_dn\n"
"    -p or --calc_pixel_num=[0 - 1000] - percent of number calc pixel\n"
"    -c or --crtc=[object_id] - crtc index\n"
"    -l or --list - show current cabc state\n");
}

#define CABC_LUT_LENGTH 128
int generate_cabc_lut_blob_from_gamma(int fd, float gamma)
{
	struct drm_mode_create_blob create_blob;
	uint32_t lut[CABC_LUT_LENGTH];
	int i, ret;

	for (i = 0; i < CABC_LUT_LENGTH; i++)
		lut[i] = pow(1.0 * (i + 128) / 255, gamma) * 4095 + 0.5;

	memset(&create_blob, 0, sizeof(create_blob));
	create_blob.length = CABC_LUT_LENGTH * sizeof(uint32_t);
	create_blob.data = (uint64_t)lut;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATEPROPBLOB, &create_blob);
	if (ret) {
		fprintf(stderr, "Failed to create mode property blob %d\n", ret);
		return ret;
	}

	return create_blob.blob_id;
}

int main(int argc, char **argv)
{
	drmModeResPtr res;
	drmModeCrtcPtr crtc = NULL;
	struct object cabc_lut, cabc_mode, stage_up, stage_down;
	struct object global_dn, calc_pixel_num;
	int crtc_index = 0;
	drmModeAtomicReq *req;
	uint32_t flags = 0;
	int i, fd, ret;
	int list_state = 0;
	float gamma;

	memset(&cabc_lut, 0, sizeof(struct object));
	memset(&cabc_mode, 0, sizeof(struct object));
	memset(&stage_up, 0, sizeof(struct object));
	memset(&stage_down, 0, sizeof(struct object));
	memset(&global_dn, 0, sizeof(struct object));
	memset(&calc_pixel_num, 0, sizeof(struct object));
	if (argc < 2)
		help();

	for (;;) {
		int c = getopt_long(argc, argv, short_options, long_options, NULL);

		if (c == -1)
			break;
		switch (c) {
		case FLAG_HELP:
			help();
			return 0;
		case FLAG_LIST:
			list_state = 1;
			break;
		case FLAG_CRTC:
			crtc_index = strtoul(optarg, NULL, 0);
			break;
		case FLAG_GAMMA:
			gamma = strtof(optarg, NULL);
			cabc_lut.changed = 1;
			break;
		case FLAG_MODE:
			cabc_mode.value = strtoul(optarg, NULL, 0);
			cabc_mode.changed = 1;
			break;
		case FLAG_STAGE_UP:
			stage_up.value = strtoul(optarg, NULL, 0);
			stage_up.changed = 1;
			break;
		case FLAG_STAGE_DOWN:
			stage_down.value = strtoul(optarg, NULL, 0);
			stage_down.changed = 1;
			break;
		case FLAG_GLOBAL_DN:
			global_dn.value = strtoul(optarg, NULL, 0);
			global_dn.changed = 1;
			break;
		case FLAG_CALC_PIXEL_NUM:
			calc_pixel_num.value = strtoul(optarg, NULL, 0);
			calc_pixel_num.changed = 1;
			break;
		}
	}

	fd = drmOpen("rockchip", NULL);
	if (fd < 0) {
		fprintf(stderr, "failed to open rockchip drm: %s\n",
			strerror(errno));
		return fd;
	}

	ret = drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);
	if (ret) {
		fprintf(stderr, "Failed to set atomic cap %s", strerror(errno));
		return ret;
	}

	if (cabc_lut.changed) {
		cabc_lut.value = generate_cabc_lut_blob_from_gamma(fd, gamma);
		if (cabc_lut.value < 0)
			return cabc_lut.value;
	}
	res = drmModeGetResources(fd);
	if (!res) {
		fprintf(stderr, "Failed to get resources: %s\n",
			strerror(errno));
		return -ENODEV;
	}

	for (i = 0; i < res->count_crtcs; ++i) {
		drmModeObjectPropertiesPtr props;
		drmModePropertyPtr prop;
		uint32_t j;

		crtc = drmModeGetCrtc(fd, res->crtcs[i]);
		if (!crtc) {
			fprintf(stderr, "Could not get crtc %u: %s\n",
					res->crtcs[i], strerror(errno));
			continue;
		}

		if (crtc_index != i)
			continue;

		props = drmModeObjectGetProperties(fd, crtc->crtc_id,
						   DRM_MODE_OBJECT_CRTC);
		if (!props) {
			fprintf(stderr, "failed to found props crtc[%d] %s\n",
				crtc->crtc_id, strerror(errno));
			continue;
		}
		for (j = 0; j < props->count_props; j++) {
			prop = drmModeGetProperty(fd, props->props[j]);
			if (!strcmp(prop->name, "CABC_LUT"))
				cabc_lut.id = prop->prop_id;
			else if (!strcmp(prop->name, "CABC_MODE"))
				cabc_mode.id = prop->prop_id;
			else if (!strcmp(prop->name, "CABC_STAGE_UP"))
				stage_up.id = prop->prop_id;
			else if (!strcmp(prop->name, "CABC_STAGE_DOWN"))
				stage_down.id = prop->prop_id;
			else if (!strcmp(prop->name, "CABC_GLOBAL_DN"))
				global_dn.id = prop->prop_id;
			else if (!strcmp(prop->name, "CABC_CALC_PIXEL_NUM"))
				calc_pixel_num.id = prop->prop_id;
			else
				continue;

			if (list_state)
				dump_prop(fd, prop->prop_id, props->prop_values[j]);
		}

		break;
	}

	if (i == res->count_crtcs) {
		fprintf(stderr, "failed to find usable crtc props\n");
		return -ENODEV;
	}

	if (list_state)
		return 0;

	req = drmModeAtomicAlloc();

#define DRM_ATOMIC_ADD_PROP(object_id, object) \
	if (object.changed && object.id) { \
		ret = drmModeAtomicAddProperty(req, object_id, object.id, object.value); \
		if (ret < 0) { \
			fprintf(stderr, "Failed to add prop[%d] to [%d]", object.id, object_id); \
			goto out; \
		} \
	}
	DRM_ATOMIC_ADD_PROP(crtc->crtc_id, cabc_mode);
	DRM_ATOMIC_ADD_PROP(crtc->crtc_id, cabc_lut);
	DRM_ATOMIC_ADD_PROP(crtc->crtc_id, stage_up);
	DRM_ATOMIC_ADD_PROP(crtc->crtc_id, stage_down);
	DRM_ATOMIC_ADD_PROP(crtc->crtc_id, global_dn);
	DRM_ATOMIC_ADD_PROP(crtc->crtc_id, calc_pixel_num);
	flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
	ret = drmModeAtomicCommit(fd, req, flags, NULL);
	if (ret)
		fprintf(stderr, "atomic: couldn't commit new state: %s\n", strerror(errno));

out:
	drmModeAtomicFree(req);

	return ret;
}
