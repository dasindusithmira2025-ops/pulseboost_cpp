import { create } from "zustand";
import type { Tweak, TweakCategory } from "../types/system";

interface TweakStore {
  tweaks: Tweak[];
  isLoading: boolean;
  applyingId: string | null;
  searchQuery: string;
  activeCategory: TweakCategory | "all";
  pulseScore: number;
  tweaksApplied: number;
  tweaksAvailable: number;
  setTweaks: (tweaks: Tweak[]) => void;
  setApplied: (id: string, applied: boolean) => void;
  setApplying: (id: string | null) => void;
  setSearch: (query: string) => void;
  setCategory: (category: TweakCategory | "all") => void;
  setPulseScore: (score: number) => void;
  filteredTweaks: () => Tweak[];
}

export const useTweakStore = create<TweakStore>((set, get) => ({
  tweaks: [],
  isLoading: true,
  applyingId: null,
  searchQuery: "",
  activeCategory: "all",
  pulseScore: 0,
  tweaksApplied: 0,
  tweaksAvailable: 0,
  setTweaks: (tweaks) =>
    set({
      tweaks,
      tweaksApplied: tweaks.filter((tweak) => tweak.isApplied).length,
      tweaksAvailable: tweaks.filter((tweak) => tweak.isApplicable).length,
      isLoading: false
    }),
  setApplied: (id, applied) =>
    set((state) => {
      const tweaks = state.tweaks.map((tweak) =>
        tweak.id === id ? { ...tweak, isApplied: applied } : tweak
      );
      return {
        tweaks,
        tweaksApplied: tweaks.filter((tweak) => tweak.isApplied).length
      };
    }),
  setApplying: (id) => set({ applyingId: id }),
  setSearch: (query) => set({ searchQuery: query }),
  setCategory: (category) => set({ activeCategory: category }),
  setPulseScore: (score) => set({ pulseScore: score }),
  filteredTweaks: () => {
    const { tweaks, searchQuery, activeCategory } = get();
    return tweaks.filter((tweak) => {
      const matchesCategory = activeCategory === "all" || tweak.category === activeCategory;
      const matchesSearch =
        !searchQuery ||
        tweak.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
        tweak.description.toLowerCase().includes(searchQuery.toLowerCase());
      return matchesCategory && matchesSearch;
    });
  }
}));
