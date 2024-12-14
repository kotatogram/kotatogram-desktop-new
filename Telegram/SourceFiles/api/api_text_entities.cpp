/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "api/api_text_entities.h"

#include "data/data_document.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "data/stickers/data_custom_emoji.h"
#include "data/stickers/data_stickers_set.h"
#include "main/main_session.h"
#include "base/qthelp_regex.h"
#include "base/qthelp_url.h"

namespace Api {
namespace {

using namespace TextUtilities;

[[nodiscard]] QString CustomEmojiEntityData(
		const MTPDmessageEntityCustomEmoji &data) {
	return Data::SerializeCustomEmojiId(data.vdocument_id().v);
}

[[nodiscard]] std::optional<MTPMessageEntity> CustomEmojiEntity(
		MTPint offset,
		MTPint length,
		const QString &data) {
	const auto parsed = Data::ParseCustomEmojiData(data);
	if (!parsed) {
		return {};
	}
	return MTP_messageEntityCustomEmoji(
		offset,
		length,
		MTP_long(parsed));
}

[[nodiscard]] std::optional<MTPMessageEntity> MentionNameEntity(
		not_null<Main::Session*> session,
		MTPint offset,
		MTPint length,
		const QString &data) {
	const auto parsed = MentionNameDataToFields(data);
	if (!parsed.userId || parsed.selfId != session->userId().bare) {
		return {};
	}
	return MTP_inputMessageEntityMentionName(
		offset,
		length,
		(parsed.userId == parsed.selfId
			? MTP_inputUserSelf()
			: MTP_inputUser(
				MTP_long(parsed.userId),
				MTP_long(parsed.accessHash))));
}

} // namespace

EntitiesInText EntitiesFromMTP(
		Main::Session *session,
		const QVector<MTPMessageEntity> &entities) {
	if (entities.isEmpty()) {
		return {};
	}
	auto result = EntitiesInText();
	result.reserve(entities.size());

	for (const auto &entity : entities) {
		entity.match([&](const MTPDmessageEntityUnknown &d) {
		}, [&](const MTPDmessageEntityMention &d) {
			result.push_back({
				EntityType::Mention,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityHashtag &d) {
			result.push_back({
				EntityType::Hashtag,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityBotCommand &d) {
			result.push_back({
				EntityType::BotCommand,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityUrl &d) {
			result.push_back({
				EntityType::Url,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityEmail &d) {
			result.push_back({
				EntityType::Email,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityBold &d) {
			result.push_back({
				EntityType::Bold,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityItalic &d) {
			result.push_back({
				EntityType::Italic,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityCode &d) {
			result.push_back({
				EntityType::Code,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityPre &d) {
			result.push_back({
				EntityType::Pre,
				d.voffset().v,
				d.vlength().v,
				qs(d.vlanguage()),
			});
		}, [&](const MTPDmessageEntityTextUrl &d) {
			result.push_back({
				EntityType::CustomUrl,
				d.voffset().v,
				d.vlength().v,
				qs(d.vurl()),
			});
		}, [&](const MTPDmessageEntityMentionName &d) {
			if (!session) {
				return;
			}
			const auto userId = UserId(d.vuser_id());
			const auto user = session->data().userLoaded(userId);
			const auto data = MentionNameDataFromFields({
				.selfId = session->userId().bare,
				.userId = userId.bare,
				.accessHash = user ? user->accessHash() : 0,
			});
			result.push_back({
				EntityType::MentionName,
				d.voffset().v,
				d.vlength().v,
				data,
			});
		}, [&](const MTPDinputMessageEntityMentionName &d) {
			if (!session) {
				return;
			}
			const auto data = d.vuser_id().match([&](
					const MTPDinputUserSelf &) {
				return MentionNameDataFromFields({
					.selfId = session->userId().bare,
					.userId = session->userId().bare,
					.accessHash = session->user()->accessHash(),
				});
			}, [&](const MTPDinputUser &data) {
				return MentionNameDataFromFields({
					.selfId = session->userId().bare,
					.userId = UserId(data.vuser_id()).bare,
					.accessHash = data.vaccess_hash().v,
				});
			}, [](const auto &) {
				return QString();
			});
			if (!data.isEmpty()) {
				result.push_back({
					EntityType::MentionName,
					d.voffset().v,
					d.vlength().v,
					data,
				});
			}
		}, [&](const MTPDmessageEntityPhone &d) {
			// Skipping phones.
		}, [&](const MTPDmessageEntityCashtag &d) {
			result.push_back({
				EntityType::Cashtag,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityUnderline &d) {
			result.push_back({
				EntityType::Underline,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityStrike &d) {
			result.push_back({
				EntityType::StrikeOut,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityBankCard &d) {
			// Skipping cards. // #TODO entities
		}, [&](const MTPDmessageEntitySpoiler &d) {
			result.push_back({
				EntityType::Spoiler,
				d.voffset().v,
				d.vlength().v,
			});
		}, [&](const MTPDmessageEntityCustomEmoji &d) {
			result.push_back({
				EntityType::CustomEmoji,
				d.voffset().v,
				d.vlength().v,
				CustomEmojiEntityData(d),
			});
		}, [&](const MTPDmessageEntityBlockquote &d) {
			result.push_back({
				EntityType::Blockquote,
				d.voffset().v,
				d.vlength().v,
			});
		});
	}
	return result;
}

MTPVector<MTPMessageEntity> EntitiesToMTP(
		not_null<Main::Session*> session,
		const EntitiesInText &entities,
		ConvertOption option) {
	auto v = QVector<MTPMessageEntity>();
	v.reserve(entities.size());
	for (const auto &entity : entities) {
		if (entity.length() <= 0) {
			continue;
		}
		if (option == ConvertOption::SkipLocal
			&& entity.type() != EntityType::Bold
			//&& entity.type() != EntityType::Semibold // Not in API.
			&& entity.type() != EntityType::Italic
			&& entity.type() != EntityType::Underline
			&& entity.type() != EntityType::StrikeOut
			&& entity.type() != EntityType::Code // #TODO entities
			&& entity.type() != EntityType::Pre
			&& entity.type() != EntityType::Blockquote
			&& entity.type() != EntityType::Spoiler
			&& entity.type() != EntityType::MentionName
			&& entity.type() != EntityType::CustomUrl
			&& entity.type() != EntityType::CustomEmoji) {
			continue;
		}

		auto offset = MTP_int(entity.offset());
		auto length = MTP_int(entity.length());
		switch (entity.type()) {
		case EntityType::Url: {
			v.push_back(MTP_messageEntityUrl(offset, length));
		} break;
		case EntityType::CustomUrl: {
			auto url = entity.data();
			auto inputUser = [&](const QString &data) -> MTPInputUser {
				const auto trimmed = url.trimmed();
				if (trimmed.isEmpty()) {
					return MTP_inputUserEmpty();
				}
				auto regex = QRegularExpression(
					QString::fromUtf8("^(?i)tg://user\\?(.+)"),
					QRegularExpression::UseUnicodePropertiesOption);
				regex.optimize();
				const auto match = regex.match(trimmed);
				if (!match.hasMatch() || match.capturedStart() != 0) {
					return MTP_inputUserEmpty();
				}
				const auto parsed = qthelp::url_parse_params(match.captured(1), qthelp::UrlParamNameTransform::ToLower);
				const auto qstr_uid = parsed.value("id");
				if (qstr_uid.isEmpty()) {
					return MTP_inputUserEmpty();
				}
				bool success;
				UserId uid = qstr_uid.toLongLong(&success);
				if (success && session) {
					if (uid == session->userId()) {
						return MTP_inputUserSelf();
					} else if (const auto user = session->data().userLoaded(uid)) {
						return MTP_inputUser(MTP_long(uid.bare), MTP_long(user->accessHash()));
					}
				}
				return MTP_inputUserEmpty();
			}(url);
			if (inputUser.type() != mtpc_inputUserEmpty) {
				v.push_back(MTP_inputMessageEntityMentionName(offset, length, inputUser));
			} else {
				v.push_back(MTP_messageEntityTextUrl(offset, length, MTP_string(url)));
			}
		} break;
		case EntityType::Email: {
			v.push_back(MTP_messageEntityEmail(offset, length));
		} break;
		case EntityType::Hashtag: {
			v.push_back(MTP_messageEntityHashtag(offset, length));
		} break;
		case EntityType::Cashtag: {
			v.push_back(MTP_messageEntityCashtag(offset, length));
		} break;
		case EntityType::Mention: {
			v.push_back(MTP_messageEntityMention(offset, length));
		} break;
		case EntityType::MentionName: {
			const auto valid = MentionNameEntity(
				session,
				offset,
				length,
				entity.data());
			if (valid) {
				v.push_back(*valid);
			}
		} break;
		case EntityType::BotCommand: {
			v.push_back(MTP_messageEntityBotCommand(offset, length));
		} break;
		case EntityType::Bold: {
			v.push_back(MTP_messageEntityBold(offset, length));
		} break;
		case EntityType::Italic: {
			v.push_back(MTP_messageEntityItalic(offset, length));
		} break;
		case EntityType::Underline: {
			v.push_back(MTP_messageEntityUnderline(offset, length));
		} break;
		case EntityType::StrikeOut: {
			v.push_back(MTP_messageEntityStrike(offset, length));
		} break;
		case EntityType::Code: {
			// #TODO entities.
			v.push_back(MTP_messageEntityCode(offset, length));
		} break;
		case EntityType::Pre: {
			v.push_back(
				MTP_messageEntityPre(
					offset,
					length,
					MTP_string(entity.data())));
		} break;
		case EntityType::Blockquote: {
			v.push_back(MTP_messageEntityBlockquote(offset, length));
		} break;
		case EntityType::Spoiler: {
			v.push_back(MTP_messageEntitySpoiler(offset, length));
		} break;
		case EntityType::CustomEmoji: {
			const auto valid = CustomEmojiEntity(
				offset,
				length,
				entity.data());
			if (valid) {
				v.push_back(*valid);
			}
		} break;
		}
	}
	return MTP_vector<MTPMessageEntity>(std::move(v));
}

} // namespace Api
