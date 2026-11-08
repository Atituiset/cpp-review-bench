// AUTO-DRAFT from torvalds/linux PR #12419f306c3ca34a38064d2b502966fa01650b49
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
  // <<< BUG ANCHOR
struct ntfs_resident_attr_value {
	const u8 *data;
	u32 len;
};
/* …（同文件无关代码省略）… */
					 struct ntfs_resident_attr_value *value)
{
	u32 attr_len;
	u16 value_offset;

	attr_len = le32_to_cpu(a->length);
	if (attr_len < offsetof(struct attr_record, data.resident.reserved) +
			sizeof(a->data.resident.reserved))
		return false;

	value->len = le32_to_cpu(a->data.resident.value_length);
	value_offset = le16_to_cpu(a->data.resident.value_offset);

	if (value->len > attr_len || value_offset > attr_len - value->len)
		return false;

	value->data = (const u8 *)a + value_offset;
