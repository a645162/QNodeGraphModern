#include <QNodeGraph/Lib/Execution/image_execution_service.h>

#include <QtConcurrent/QtConcurrentRun>

#include <QString>
#include <utility>

namespace QNodeGraph::Execution {

ImageExecutionService::ImageExecutionService(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<Image::ImageFrame>();
}

quint64 ImageExecutionService::submit(const Image::ImageFrame& input,
                                      Processor processor) {
    if (!processor) {
        return 0;
    }
    const auto requestId = m_nextRequestId.fetch_add(1);
    auto* watcher = new Watcher(this);
    m_watchers.emplace(requestId, watcher);

    connect(watcher, &Watcher::finished, this,
            [this, requestId, watcher]() {
                const auto iterator = m_watchers.find(requestId);
                if (iterator == m_watchers.end()) {
                    watcher->deleteLater();
                    return;
                }
                const auto result = watcher->result();
                m_watchers.erase(iterator);
                watcher->deleteLater();
                if (m_cancelled.erase(requestId) > 0) {
                    return;
                }
                if (result) {
                    emit finished(requestId, *result);
                } else {
                    emit failed(requestId,
                                QString::fromStdString(result.error().message));
                }
            });

    watcher->setFuture(QtConcurrent::run(
        [input, processor = std::move(processor)]() mutable {
            return processor(input);
        }));
    return requestId;
}

bool ImageExecutionService::cancel(quint64 requestId) {
    if (!m_watchers.contains(requestId)) {
        return false;
    }
    m_cancelled.insert(requestId);
    return true;
}

int ImageExecutionService::pendingCount() const noexcept {
    return static_cast<int>(m_watchers.size());
}

} // namespace QNodeGraph::Execution

