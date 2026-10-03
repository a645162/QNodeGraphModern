#pragma once

#include <QNodeGraph/Lib/Image/image_frame.h>

#include <QFutureWatcher>
#include <QObject>
#include <QVariant>

#include <atomic>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace QNodeGraph::Execution {

class ImageExecutionService final : public QObject {
    Q_OBJECT

public:
    using Result = Core::GraphResult<Image::ImageFrame>;
    using Processor = std::function<Result(const Image::ImageFrame&)>;
    using Watcher = QFutureWatcher<Result>;

    explicit ImageExecutionService(QObject* parent = nullptr);

    [[nodiscard]] quint64 submit(const Image::ImageFrame& input,
                                 Processor processor);
    [[nodiscard]] bool cancel(quint64 requestId);
    [[nodiscard]] int pendingCount() const noexcept;

signals:
    void finished(quint64 requestId, QNodeGraph::Image::ImageFrame result);
    void failed(quint64 requestId, QString message);

private:
    std::atomic<quint64> m_nextRequestId{1};
    std::unordered_map<quint64, Watcher*> m_watchers;
    std::unordered_set<quint64> m_cancelled;
};

} // namespace QNodeGraph::Execution

Q_DECLARE_METATYPE(QNodeGraph::Image::ImageFrame)

