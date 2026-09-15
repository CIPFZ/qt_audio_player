#pragma once
#include <QStringList>
#include <atomic>
#include <memory>
QStringList scanAudioPaths(const QStringList &inputs, const std::shared_ptr<std::atomic_bool> &cancelled);
