import { useState, useRef } from 'react';
import { useQuery } from '@tanstack/react-query';
import { sessionsApi } from '../api/client';

function AudioPlayer({ sessionId }) {
  const [error, setError] = useState(false);
  const ref = useRef(null);

  if (error) return <span className="text-gray-400 text-sm">Keine Aufnahme</span>;

  return (
    <audio ref={ref} controls className="w-full" onError={() => setError(true)}>
      <source src={sessionsApi.getAudioUrl(sessionId)} type="audio/wav" />
    </audio>
  );
}

export default function SessionsPage() {
  const [selected, setSelected] = useState(null);

  const { data, isLoading } = useQuery({
    queryKey: ['sessions'],
    queryFn: () => sessionsApi.getAll({ limit: 50, offset: 0 })
  });

  const { data: interactions } = useQuery({
    queryKey: ['interactions', selected],
    queryFn: () => sessionsApi.getInteractions(selected),
    enabled: !!selected
  });

  if (isLoading) return <div className="text-gray-500 py-8">Lade Sessions...</div>;

  const sessions = data?.data || [];

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-gray-900">Sessions</h2>
        <span className="text-sm text-gray-500">{sessions.length} gefunden</span>
      </div>

      <div className="bg-white rounded-lg shadow border overflow-hidden">
        <table className="min-w-full divide-y divide-gray-200">
          <thead className="bg-gray-50">
            <tr>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">ID</th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Sprache</th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Status</th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Zeit</th>
              <th className="px-6 py-3"></th>
            </tr>
          </thead>
          <tbody className="divide-y divide-gray-200">
            {sessions.map(s => (
              <tr key={s.session_id} className="hover:bg-gray-50">
                <td className="px-6 py-4 font-mono text-sm">{s.session_id}</td>
                <td className="px-6 py-4">
                  <span className="px-2 py-1 rounded text-xs bg-blue-100 text-blue-800">
                    {s.language.toUpperCase()}
                  </span>
                </td>
                <td className="px-6 py-4">
                  <span className={`px-2 py-1 rounded-full text-xs ${
                    s.status === 'completed' ? 'bg-green-100 text-green-800' : 'bg-red-100 text-red-800'
                  }`}>
                    {s.status}
                  </span>
                </td>
                <td className="px-6 py-4 text-sm text-gray-500">
                  {new Date(s.started_at).toLocaleString('de-DE')}
                </td>
                <td className="px-6 py-4">
                  <button
                    onClick={() => setSelected(s.session_id)}
                    className="text-blue-600 hover:text-blue-800 text-sm"
                  >
                    Details
                  </button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      {selected && interactions && (
        <div className="bg-white rounded-lg shadow border p-6">
          <div className="flex justify-between items-center mb-4">
            <h3 className="text-lg font-semibold">Session {selected}</h3>
            <button onClick={() => setSelected(null)} className="text-gray-500 hover:text-gray-700">
              Schließen
            </button>
          </div>

          <div className="mb-4 p-3 bg-blue-50 rounded">
            <AudioPlayer sessionId={selected} />
          </div>

          <h4 className="font-medium mb-3">Interaktionen ({interactions.data.length})</h4>
          <div className="space-y-3">
            {interactions.data.map((i, idx) => (
              <div key={i.id} className="border rounded p-3">
                <div className="flex justify-between text-sm text-gray-500 mb-2">
                  <span>#{idx + 1}</span>
                  <span>{i.total_processing_time_ms}ms</span>
                </div>
                <p className="text-sm"><b>Input:</b> {i.transcribed_text || '-'}</p>
                <p className="text-sm"><b>Output:</b> {i.rasa_response || '-'}</p>
                <p className="text-xs text-gray-500 mt-1">
                  Intent: {i.rasa_intent} ({i.rasa_confidence})
                </p>
                {i.error_occurred && (
                  <p className="text-xs text-red-600 mt-1">Fehler: {i.error_message}</p>
                )}
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}
