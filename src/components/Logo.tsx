export function Logo({ small = false }: { small?: boolean }) {
  return (
    <div className={`brand ${small ? 'brand-small' : ''}`}>
      <svg className="brand-mark" viewBox="0 0 34 34" fill="none" aria-hidden="true">
        <path d="M4 25.5 13.7 7l6.1 10.1L23.3 12 30 25.5H4Z" stroke="currentColor" strokeWidth="1.6" strokeLinejoin="round" />
        <path d="m9.2 25.5 7.2-12 4.1 6.6 3.3-3.7" stroke="currentColor" strokeWidth="1.25" strokeLinejoin="round" />
        <path d="M4 29h26" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" />
      </svg>
      {!small && <span>FRONTIER</span>}
    </div>
  )
}
