/* This file is part of Clementine.
 Copyright 2014, Andre Siviero <altsiviero@gmail.com>

 Clementine is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Clementine is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SRC_RIPPER_RIPPER_H_
#define SRC_RIPPER_RIPPER_H_

#include <cdio/cdio.h>

#include <QMutex>
#include <QObject>
#include <QVector>

#include "core/song.h"
#include "core/tagreaderclient.h"
#include "transcoder/transcoder.h"

class QFile;

// Rips selected tracks from an audio CD, transcodes them to a chosen
// format, and finally tags the files with the supplied metadata.
//
// Usage: Add tracks with AddTrack() and album metadata with
// SetAlbumInformation(). Then start the ripper with Start(). The ripper
// emits the Finished() signal when it's done or the Cancelled()
// signal if the ripping has been cancelled.
class Ripper : public QObject {
  Q_OBJECT

 public:
  explicit Ripper(QObject* parent = nullptr);
  ~Ripper();

  // Adds a track to the rip list if the track number corresponds to a
  // track on the audio cd. The track will transcoded according to the
  // chosen TranscoderPreset.
  void AddTrack(int track_number, const QString& title,
                const QString& transcoded_filename,
                const TranscoderPreset& preset);
  // Sets album metadata. This information is used when tagging the
  // final files.
  void SetAlbumInformation(const QString& album, const QString& artist,
                           const QString& genre, int year, int disc,
                           Song::FileType type);
  // Returns the number of audio tracks on the disc.
  int TracksOnDisc() const;
  // Returns the number of tracks added to the rip list.
  int AddedTracks() const;
  // Clears the rip list.
  void ClearTracks();
  // Returns true if a cd device was successfully opened.
  bool CheckCDIOIsValid();
  // Returns true if the cd media has changed.
  bool MediaChanged() const;

 signals:
  void Finished();
  void Cancelled();
  void ProgressInterval(int min, int max);
  void Progress(int progress);
  void RippingComplete();

 public slots:
  void Start();
  void Cancel();

 private slots:
  void TranscodingJobComplete(const QString& input, const QString& output,
                              bool success);
  void AllTranscodingJobsComplete();
  void LogLine(const QString& message);
  void FileTagged(TagReaderReply* reply);

 private:
  struct TrackInformation {
    TrackInformation(int track_number, const QString& title,
                     const QString& transcoded_filename,
                     const TranscoderPreset& preset)
        : track_number(track_number),
          title(title),
          transcoded_filename(transcoded_filename),
          preset(preset) {}

    int track_number;
    QString title;
    QString transcoded_filename;
    TranscoderPreset preset;
    QString temporary_filename;
  };

  struct AlbumInformation {
    AlbumInformation() : year(0), disc(0), type(Song::Type_Unknown) {}

    QString album;
    QString artist;
    QString genre;
    int year;
    int disc;
    Song::FileType type;
  };


  struct RippingProgress {
    RippingProgress(int num_tracks) noexcept;
    RippingProgress(const RippingProgress& other) noexcept;
    RippingProgress(RippingProgress&& other) noexcept;

    void swap(RippingProgress& other) noexcept;
    RippingProgress& operator=(RippingProgress other) noexcept;

    int current_progress;
    int finished_success;
    int finished_failed;
    QVector<float> per_track_ripping_progress;
    QVector<float> per_track_transcoding_progress;
    QMutex mutex;
  };

  void WriteWAVHeader(QFile* stream, int32_t i_bytecount);
  void Rip();
  // Emits min and max values for the progress interval as well as a current progress of 0.
  void SetupProgressInterval();

  void RemoveTemporaryDirectory();
  void TagFiles();

  // Updates progress for initial ripping of a track from the disc.
  void UpdateRippingProgress(int jobId, int jobStart, int jobEnd, int jobCurrent);
  // Updates progress of transcoder.
  void UpdateTranscodingProgress();
  // Periodically polls progress from transcoder by invoking UpdateTranscodingProgress.
  void PollTranscodingProgress();

  CdIo_t* cdio_;
  Transcoder* transcoder_;
  QString temporary_directory_;
  bool cancel_requested_;
  bool transcoding_active_;
  QMutex mutex_;
  RippingProgress progress_;
  int files_tagged_;
  QList<TrackInformation> tracks_;
  AlbumInformation album_;
};

#endif  // SRC_RIPPER_RIPPER_H_
