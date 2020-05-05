extern "C"
{
#include "libavutil/log.h"
#include "libavutil/error.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
}

static const char* type_string(int type)
{
	switch (type) {
	case AVIO_ENTRY_DIRECTORY:
		return "<DIR>";
	case AVIO_ENTRY_FILE:
		return "<FILE>";
	case AVIO_ENTRY_BLOCK_DEVICE:
		return "<BLOCK DEVICE>";
	case AVIO_ENTRY_CHARACTER_DEVICE:
		return "<CHARACTER DEVICE>";
	case AVIO_ENTRY_NAMED_PIPE:
		return "<PIPE>";
	case AVIO_ENTRY_SYMBOLIC_LINK:
		return "<LINK>";
	case AVIO_ENTRY_SOCKET:
		return "<SOCKET>";
	case AVIO_ENTRY_SERVER:
		return "<SERVER>";
	case AVIO_ENTRY_SHARE:
		return "<SHARE>";
	case AVIO_ENTRY_WORKGROUP:
		return "<WORKGROUP>";
	case AVIO_ENTRY_UNKNOWN:
	default:
		break;
	}
	return "<UNKNOWN>";
}

int avio_list_dir(const char *dir_name)
{
	av_log_set_level(AV_LOG_DEBUG);

	avformat_network_init();

	int ret = 0;
	char err_buf[64] = { 0 };
	int cnt = 0;
	AVIODirContext* ctx = NULL;
	if ((ret = avio_open_dir(&ctx, dir_name, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Can't open directory %s: %s\n", 
			dir_name, av_make_error_string(err_buf, sizeof(err_buf), ret));
		goto fail;
	}

	while (1)
	{
		AVIODirEntry* entry = NULL;
		if ((ret = avio_read_dir(ctx, &entry)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot list directory: %s.\n", av_make_error_string(err_buf, sizeof(err_buf), ret));
			goto fail;
		}

		if (entry == NULL)
			break;

		char file_mode[4] = { 0 };
		if (entry->filemode == -1)
		{
			snprintf(file_mode, 4, "????");
		}
		else
		{
			snprintf(file_mode, 4, "%3llo", entry->filemode);
		}
		char uid_and_gid[20] = { 0 };
		snprintf(uid_and_gid, 20, "%lld(%lld)", entry->user_id, entry->group_id);
		if(cnt == 0)
			av_log(NULL, AV_LOG_INFO, "%-9s %12s %30s %10s %s %16s %16s %16s\n",
				"TYPE", "SIZE", "NAME", "UID(GID)", "UGO", "MODIFIED",
				"ACCESSED", "STATUS_CHANGED");
		av_log(NULL, AV_LOG_INFO, "%-9s %12lld %30s %10s %s %16lld %16lld %16lld\n",
			type_string(entry->type),
			entry->size,
			entry->name,
			uid_and_gid,
			file_mode,
			entry->modification_timestamp,
			entry->access_timestamp,
			entry->status_change_timestamp);
		avio_free_directory_entry(&entry);
		cnt++;
	}

fail:
	avio_close_dir(&ctx);

	avformat_network_deinit();

	return 0;
}