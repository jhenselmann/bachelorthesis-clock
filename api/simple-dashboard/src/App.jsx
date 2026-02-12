import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import SessionsPage from './pages/SessionsPage';

const queryClient = new QueryClient();

export default function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <div className="min-h-screen bg-gray-50">
        <header className="bg-gray-800 shadow-lg">
          <div className="max-w-7xl mx-auto px-4 py-4">
            <h1 className="text-xl font-bold text-white">SmUhr Dashboard</h1>
          </div>
        </header>
        <main className="max-w-7xl mx-auto px-4 py-8">
          <SessionsPage />
        </main>
      </div>
    </QueryClientProvider>
  );
}
